(sorry, in french for the moment)

# Idée de layout pour les fichiers dans une installation classique :

Le layout tel que peut le constater l'utilisateur :

- `io:/` : périphériques système et accès aux ressources contrôlées
  par le processus parent, peut changer quand on entre dans une "boîte".
  Lecture seule, contôlé par l'init et le desktop manager (ou n'importe
  quel processus qui crée une "boîte")
  - `io:/keyboard`, `io:/mouse`, `io:/display`, `io:/sound` : interfaces spécialisées
    sur chacun, accessibles via une API fournie dans `libkogata`

- `root:/` : partition système. Contenu géré par l'administrateur du système.
  - `root:/boot/kernel-2.bin` : le noyau (V2)
  - `root:/boot/init-4.bin` : l'init et gestionnaire de paquets système (V4)
  - `root:/pkg/` : les paquets du système, montés en overlay sur `sys:` et sur `cmd:`
      - `root:/pkg/kogata` : paquets essentiels du système
          - `root:/pkg/kogata/kogata-r2.pkg` : release 2 du système kogata de 
            base (contenu à choisir précisément)
          - `root:/pkg/kogata/kogata-compat-r2.pkg` : librairies compatibles avec
            les anciennes API mais fonctionnant avec le système R2 actuel
          - `root:/pkg/kogata/kogata-dev-r2.pkg` : binaires ligne de commande
            et en-têtes de dév pour la release 2
          - `root:/pkg/kogata/kogata-extras-r2.pkg` : ressources, bibliothèques
            et utilitaires supplémentaires
          - `root:/pkg/kogata/kogata-langpack-ja-r2.pkg` : fichiers nécessaires
            pour utiliser le système en japonais (au pif !)
      - `root:/pkg/themes/` : fichiers de personnalisation de l'interface :
        polices, icônes, fonds d'écran
      - `root:/pkg/filetypes/` : extensions à libfiles pour la gestion
        de types de fichiers nouveaux (images, vidéo, ...)
  - `root:/apps/` : applications et utilitaires supplémentaires, ne peuvent pas dépendre de
    bibliothèques non inclues  dans `kogata-rx.pkg` ou `kogata-extras-rx.pkg`
    et doivent inclure elle-même les fichiers `.so` correspondant
  - `root:/services/` : applications du serveur
  - `root:/config/` : configurations du système (plusieurs sous-dossiers pour plusieurs configurations)
  - `root:/users/` : à défaut d'utiliser une partition séparée, dossiers des utilisateurs
  - `root:/services/` : données des serveurs

- `config:/` : bind-mount de `root:/config/config-en-cours`, contient les
  fichiers de configuration du système

- `sys:/` : librairies, binaires et ressources du système, c'est un overlayfs
  constitué à partir d'images pré-préparées contenues dans `root:/pkg/`.
  Tout ce qui est dans `sys:` est considéré de source sûre.
  - `sys:/lib` : librairies
      - `sys:/lib/libkogata/libkogata-1.so` : `libkogata` avec compatibilité API V1
      - `sys:/lib/libkogata/libkogata-dev.so` : `libkogata` sans compatibilité (version de développement)
      - `sys:/lib/libkogata/libkogata-sys.so` : `libkogata` avec compatibilité
        API avec les binaires du système
      - `sys:/lib/libc/libc-1.so`
      - `sys:/lib/libalgo/libalgo-1.so`
      - `sys:/lib/libgui` : bibliothèque de dessin à l'écran, rendu des polices, toolkit graphique
      - `sys:/lib/libfiles` : bibliothèque pour la manipulation de fichiers de différents formats ; extensible
  - `sys:/filetypes` : extensions à `libfiles` pour gérer des fichiers
      - `sys:/filetypes/jpeg-1.so` : plugin pour charger des JPEG
      - `sys:/filetypes/mp3-1.so` : plugin pour charger des MP3
  - `sys:/fonts` : extensions à `libgui`, polices d'écriture
  - `sys:/input` : extensions à `libgui`, méthodes d'entrée et keymaps
      - `sys:/input/keymaps` : keymaps
  - `sys:/icons` : extensions à `libgui`, thèmes d'icônes
      - `sys:/icons/default` : icônes par défaut
  - `sys:/wallpapers` : fonds d'écran
  - `sys:/bin` : binaires du système
      - `sys:/bin/login.bin` : login manager
      - `sys:/bin/desktop.bin` : le gestionnaire de bureau
        (wm-compositor, multiplexage des périphériques, boxing,
        IPC-master & gestionnaire d'autorisations)
      - `sys:/bin/config.bin` : éditeur de configuration utilisateur
      - `sys:/bin/admin.bin` : éditeur de configuration système et autres outils d'admin
  - `sys:/cmd` : binaires utilisables en ligne de commande, essentiellement pour le dév ;
    hiérarchie type unix
      - `sys:/cmd/bin/`
      - `sys:/cmd/lib/`
      - `sys:/cmd/include/`
      - `sys:/cmd/share/`

- `apps:/` : dossier contenant les paquets d'application pour les applications utilisateurs,
  c'est un bind-mount de `root:/apps` essentiellement
- `services:/` : dossier contenant les paquets d'application pour les services du système,
  bind-mount de `root:/services`
- `user:/` : bind-mount de `root:/users/utilisateur-actuel`, lecture-écriture,
  géré à la discrétion de l'utilisateur
  - `user:/apps` : paquets d'application
  - `user:/data` : configuration et données des applications

Ligne d'init typique :

    kernel /boot/kernel-12.bin root=io:/disk/ata0p0 init=root:/boot/init-12.bin config=test-config

Hiérarchie telle que la voit une application qui tourne dans une boîte
(c'est-à-dire n'importe quelle application de `apps:/` ou `home:/apps`) :

- `io:/` : accès contrôlé aux ressources du système et à l'extérieur
  - `io:/display`, `io:/mouse`, ... accessible via une inter-couche dans `desktop.bin` qui gère les autorisations
  - `io:/desktop` : communication contrôlée avec les autres processus (notifications, demandes d'accès, ...)
- `sys:/` : accès en lecture seule permettant d'utiliser les libs du système
- `app:/` : dossier application (lecture-seule, correspond à `apps:/nom-appli.pkg` ou `user:/apps/nom-appli.pkg`)
- `data:/` : dossier de données de l'application pour l'utilisateur
  (lecture-écriture, correspond à `user:/data/nom-appli`)
- `user:/` : sous-portion séléctivement autorisée en lecture seule et en lecture-écriture du dossier `user:/`

Hiérarchie telle que la voit un service du système qui tourne dans une boîte
(c'est-à-dire n'importe quel serveur)

- `sys:/` : accès en lecture seule permettant d'utiliser les libs du système
- `app:/` : dossier du service, correspond à `services:/nom-service.pkg`
- `data:/` : bind-mount de `root:/services/nom-service`

Du coup ce qu'il faut en termes de systèmes de fichiers :

- il faut que seul le processus qui a créé un FS puisse l'adminsitrer (ie faire des add-source)
- `pkgfs` : capable de prendre un nombre arbitraire de fichiers `.pkg`
  et de faire apparaître leur contenu comme un dossier
  - il faut que les fichiers `.pkg` puissent facilement contenir des métadonnées
  - au moment du mount, soit copier en RAM (typiquement pour les paquets système
    ou pour les applis utilisateurs dans certains cas), soit lire à la demande à
    partir du support sous-jaccent
  - toujours read-only
  - charger un `.pkg` correspond à un add-source
- Nativement dans la couche VFS inclure une fonctionnalité `fs_subfs` qui permet de :
  - prendre un sous-dossier comme racine
  - restreindre les droits (ie enlver tous droits d'écriture, typiquement)
  - dans un premier temps, cette seconde fonctionnalité peut suffire,
    on n'utilise simplement pas encore de fichiers `.pkg` mais par exemple un cdrom
    avec un dossier `sys` que l'on binde comme `sys:/` par exemple.

Rôle des différents binaires du système :

- `init.bin` : fait le setup de `config:/` puis de `sys:/`, `apps:/` et
  `services:/`, lance `sys:/bin/login.bin` avec tous les droits (pour les
  services on verra plus tard mais il y aura sans doute un
  `sys:/bin/svcmgr.bin`)
- `login.bin` : authentification, fait le setup de `user:/`, puis lance
  `sys:/bin/desktop.bin` avec les droits associés à l'utilisateur (`sys` et
  `config` en RO, éventuellement déjà un partage des périphériques pour prévoir
  plusieurs sessions simultanées - un `login.bin` primitif ne fait rien de tout
  ça évidemment)
- alternativement `login.bin` peut lancer `admin.bin` (console
  d'administration) avec tous les droits, sur authentification appropriée
- `desktop.bin` : gère les périphériques au niveau utilisateur, crée des boîtes
  pour lancer les applications utilisateur


