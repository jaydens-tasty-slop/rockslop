void Help_screen1() {
  gb.display.clear();
  gb.display.print("Le Sokoban est un\n");
  gb.display.print("jeu de reflexion\n");
  gb.display.print("japonais dont le\n");
  gb.display.print("but est de pousser\n");
  gb.display.print("les caisses d un\n");
  gb.display.print("entrepot pour les\n");
  gb.display.print("ranger sur des\n");
  gb.display.print("marques sur le sol.\n\n");
  gb.display.print("-> Suiv. <B> Retour");
  if (gb.buttons.pressed(BUTTON_RIGHT)) {
      ++game.HELP_PAGE;
  }
    if (gb.buttons.pressed(BUTTON_B)) {
      game.state = State::startmenu;
  }
};

void Help_screen2() {
  gb.display.clear();
  gb.display.print("Mais attention :\n\n");
  gb.display.print("- On ne peut que\n");
  gb.display.print("pousser qu une\n");
  gb.display.print("caisse a la fois\n");
  gb.display.print("- On ne peut ni les\n");
  gb.display.print("tirer, ni passer au\n");
  gb.display.print("dessus\n\n");
  gb.display.print(" <-  ->   <B> Retour");
  
  if (gb.buttons.pressed(BUTTON_RIGHT)) {
      ++game.HELP_PAGE;
  }
  if (gb.buttons.pressed(BUTTON_LEFT)) {
      --game.HELP_PAGE;
  }
    if (gb.buttons.pressed(BUTTON_B)) {
      game.state = State::startmenu;
  }
};

void Help_screen3() {
  gb.display.clear();
  gb.display.print("Deplacez vous a l\n");
  gb.display.print("aide des fleches\n\n");
  gb.display.print("Durant le jeu: \n\n");
  gb.display.print("A:Affiche la carte\n\n");
  gb.display.print("B:Recommence niveau\n\n");
  gb.display.print("<- Prec.  <B> Retour");
  
  if (gb.buttons.pressed(BUTTON_LEFT)) --game.HELP_PAGE;
  if (gb.buttons.pressed(BUTTON_B)) game.state = State::startmenu;
};
