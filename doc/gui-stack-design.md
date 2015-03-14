(sorry, in french for the moment)

GUI stack design
================

Two protocols :

- GIP (Graphics Initiation Protocol) : one GIP session handles one graphical
  framebuffer, a keyboard, a mouse. The buffer is created by the server and
  shared to the client with a token (a kernel IPC mechanism for file
  descriptors). Reset operation. Possible extention for audio input/output to be
  done at this level.
- WMP (Window Management Protocol) : creation of windows and of a GIP channel
  for interacting with each window. Usual window managing messages. Hierarchy of
  WMP channels : in a WMP channel you can ask for the creation of a sub-channel
  ; when a channel is closed all the sub-channels are closed too.

Software components of the GUI stack :

- `giosrv` handles the i/o with the hardware and makes that available as a GIP
  channel
- login manager : talks to `giosrv`, displays the login manager, and once a
  user is logged in, simply proxies the GIP between `giosrv` and the user
  desktop manager. Intercepts Ctrl+Alt+Del to lock the current session and go
  back to login screen.
- sessin manager : instantiated by login manager, runs with the privileges of
  the user. Window managing.
- graphical toolkit library


----

Version française.

Deux protocoles :

- GIP (Graphics Initiation Protocol) : un buffer graphique, un clavier, une
  souris. Redimensionnement. Création du buffer partagé. Opération de reset.
  Extension possible de GIP pour le son.
- WMP (Window Management Protocol) : création de fenêtres et d'un canal GIP pour
  chaque fenêtre. Déplacement, barres de titre, icônes, propriétés quelconques,
  messages, notifications, ... Gère une hiérarchie de canaux WMP : dans un canal
  WMP on peut demander la création d'un nouveau canal WMP qui sera créé comme
  fils du canal actuel.

Pile graphique :

- Serveur graphique racine : parle au matériel, fournit un canal GIP (root GIP).
- Login manager : parle au root GIP, a sa propre UI pour lancer des sessions et
  compagnie, crée pour chaque sous-session un canal GIP particulier (session
  GIP).
- Session manager : parle au login manager, a accès au session GIP, gère les
  fenêtres et propose des canaux WMP pour les applications de la session (ce qui
  engendre la création de canaux window GIP pour chaque fenêtre)
- Bibliothèque "libtk" : sur une session GIP donner une interface facile pour
  créer des widgets, gérer des layouts de widgets, gérer les interactions
  (transformation des keycodes en lettres, etc.)

Applications système :

- Desktop manager : lancé par le session manager, affiche l'interface
  utilisateur de base. C'est le client du WMP racine pour la session. Inclut la
  portion "gestionnaire de fichier" de l'UI.
- Terminal : application qui peut tourner sur un simple GIP, en particulier en
  plein écran dans une session administrateur (ou directement sur le serveur
  racine tant qu'on n'aura pas écrit le login manager). Crée un canal
  pur-textuel (type VT100) pour les applications console à l'intérieur.

Deux types d'application :

- Des applications WMP qui désirent utiliser plusieurs fenêtres (la majorité des
  cas). S'attendent à être lancées avec comme file descriptor 0 un canal WMP.
- Des applications GIP qui utilisent une seule fenêtre (typiquement, les jeux).
  S'attendent à être lancées avec comme FD0 un canal GIP.

Le desktop manager, lorsqu'il lance une application, doit demander la création
d'un nouveau canal WMP pour cette application. De plus si l'application indique
que c'est une simple application GIP alors le desktop manager s'occupe du
dialogue sur ce canal WMP particulier pour la création d'une fenêtre et du
lancement du GIP correspondant pour l'application. Il peut ainsi binder le bon
FD0 sur le process de l'application lancée.

Hiérarchie de raccourcis claviers et d'interceptions :

- Quand on est dans une session utilisateur, le login manager agit comme un
  proxy GIP. Il intercepte simplement le raccourci Ctrl+Alt+Del, qui renvoie sur
  l'interface du gestionnaire de session (changer d'utilisateur, ou bien tuer
  des sessions ou lancer les outils d'administration)
- Quand on est dans une session utilisateur, le window manager (session manager)
  intercepte tous les raccourcis avec la touche "super". La plupart sont
  directement liés au window-management, mais on peut imaginer un raccourci qui
  lance une interface qui liste la hiérarchie de toutes les sessions WMP actives
  et permette de les tuer sélectivement, neutralisant ainsi les applications qui
  spamment l'interface de messages débiles.

