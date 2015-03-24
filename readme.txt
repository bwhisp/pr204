Bonjour, 

vous trouverez dans ce dossier les sources pour le projet. Ce fichier texte donne des précisions sur le fonctionnement du code, plus particulièrement sur le lancement. 


Sommaire

	1. Lancer le programme de dsm
	2. Qu'est ce qui a été fait
	3. Test des differentes étapes


1. Lancer le programme de dsm
	Il faut que les executables à lancer à distance se trouvent dans le home directory des machines distantes (dsmwrap, truc et exemple), l'executable de dsmexec doit se trouver dans le dossier bin. Pour cela ajouter l'alias suivant "alias maki="make && cp -v bin/* $HOME/".
	Après avoir compilé, la ligne de commande à executer pour lancer le programme de test de la phase 1 est "bin/dsmexec machine_file ./truc [arguments de truc]". Pour tester la phase 2, la ligne est "bin/dsmexec machine_file ./exemple". 


2. Qu'est ce qui a été fait
	Nous nous sommes arrêtés à l'interconnexion entre les machines distantes dans la phase 2. Un traitant de signal pour SIGSEGV à été commencé mais n'a pas été testé. 
	La phase 1 est complète, tous les commits (effectués avec github) sont disponibles à l'adresse suivante : https://github.com/bwhisp/pr204.git.

3. Test des differentes étapes
	Les codes servant à tester le déroulement des differentes étapes (par exemple les envois des infos aux machines distantes) sont commentés lorsque nous estimions qu'il "polluaient" le terminal. 
