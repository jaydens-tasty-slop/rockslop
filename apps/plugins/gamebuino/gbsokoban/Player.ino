// Déclaration de nos 2 objets mobiles: Le joueur et une caisse
Type_Joueur Joueur;
Type_Caisse BougeCaisse;


// Procédure de recherche de la position initiale du joueur (le @ dans le tableau)
void trouve_position_perso() {
  for (int ligne = 0; ligne < NB_LIGNES_NIVEAUX; ligne++) {
    for (int colonne = 0; colonne < NB_COLONNES_NIVEAUX; colonne++) {
      if (currentLevelData[ligne * NB_COLONNES_NIVEAUX + colonne] == '@') {
        Joueur.X = colonne;
        Joueur.Y = ligne;
      }
    }
  }
}


void MAJ_Joueur() {
  if (Joueur.En_mouvement != STOP) {   // Si le joueur est en mouvement
    Joueur.Anime += 2;                 // On avance dans l'animation du mouvement   
    if (Joueur.Anime > (TailleSprite * VITESSE_ANIME)) {  // VITESSE_ANIME permet de régler la vitesse du déplacement en restant immobile dans une boucle
      // On retire dans le tableau du niveau le personnage de la position où il se trouvait
      switch (currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X]) {
        case '@':
          currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X] = ' ';
          break;
        case '+':
          currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X] = '.';
          break;
      }

      // Si le joueur poussait une caisse on retire la caisse de son ancien emplacement
      if (Joueur.Pousse == 1) {
        switch (currentLevelData[(Joueur.Y + BougeCaisse.Y) * NB_COLONNES_NIVEAUX + Joueur.X + BougeCaisse.X]) {
          case '$':
            currentLevelData[(Joueur.Y + BougeCaisse.Y) * NB_COLONNES_NIVEAUX + Joueur.X + BougeCaisse.X] = ' ';
            break;
          case '*':
            currentLevelData[(Joueur.Y + BougeCaisse.Y) * NB_COLONNES_NIVEAUX + Joueur.X + BougeCaisse.X] = '.';
            break;
        }
      }

      // On calcule la nouvelle position du joueur
      
      switch (Joueur.En_mouvement) {
        case BAS :
          if (Joueur.Y < NB_LIGNES_NIVEAUX) Joueur.Y++;
          break;
        case HAUT :
          if (Joueur.Y > 1) Joueur.Y--;
          break;
        case DROITE :
          if (Joueur.X < NB_COLONNES_NIVEAUX)Joueur.X++;
          break;
        case GAUCHE :
          if (Joueur.X > 1) Joueur.X--;
          break;
      }

      // On enregistre cette nouvelle poition du joueur dans le tableau du niveau
      switch (currentLevelData[Joueur.Y* NB_COLONNES_NIVEAUX + Joueur.X ]) {
        case ' ':
         currentLevelData[Joueur.Y* NB_COLONNES_NIVEAUX + Joueur.X ]  = '@';
          break;
        case '.':
          currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X ] = '+';
          break;
      }

      // Si le joueur poussait une caisse, on actualise la nouvelle position de la caisse dans le tableau du niveau
      if (Joueur.Pousse == 1) {
        switch (currentLevelData[(Joueur.Y + BougeCaisse.Y)* NB_COLONNES_NIVEAUX + Joueur.X + BougeCaisse.X]) {
          case ' ':
            currentLevelData[(Joueur.Y + BougeCaisse.Y)* NB_COLONNES_NIVEAUX + Joueur.X + BougeCaisse.X] = '$';
            break;
          case '.':
            currentLevelData[(Joueur.Y + BougeCaisse.Y)* NB_COLONNES_NIVEAUX + Joueur.X + BougeCaisse.X] = '*';
            break;
        }
      } 
      
      // On met à jour les variables indiquant que le mouvement est terminé
      Joueur.En_mouvement = STOP; 
      Joueur.Anime = 0;
      Joueur.Pousse = 0; BougeCaisse.X = 0; BougeCaisse.Y = 0;
    }
  } else {
    if (gb.buttons.pressed(BUTTON_UP)) {
      Joueur.Direction = HAUT;
      if (currentLevelData[(Joueur.Y - 1) * NB_COLONNES_NIVEAUX + Joueur.X ] == ' '   || currentLevelData[(Joueur.Y - 1) * NB_COLONNES_NIVEAUX + Joueur.X ] == '.') Joueur.En_mouvement = HAUT;
      if (currentLevelData[(Joueur.Y - 1) * NB_COLONNES_NIVEAUX + Joueur.X ] == '$'   || currentLevelData[(Joueur.Y - 1) * NB_COLONNES_NIVEAUX + Joueur.X ] == '*') {
        if (currentLevelData[(Joueur.Y - 2) * NB_COLONNES_NIVEAUX + Joueur.X ] == ' ' || currentLevelData[(Joueur.Y - 2) * NB_COLONNES_NIVEAUX + Joueur.X ] == '.') {
          Joueur.En_mouvement = HAUT;
          BougeCaisse.Y = -1;
          Joueur.Pousse = 1;
        }
      }
    }
    if (gb.buttons.pressed(BUTTON_DOWN)) {
      Joueur.Direction = BAS;
      if (currentLevelData[(Joueur.Y + 1) * NB_COLONNES_NIVEAUX + Joueur.X ] == ' ' || currentLevelData[(Joueur.Y + 1) * NB_COLONNES_NIVEAUX + Joueur.X ] == '.') Joueur.En_mouvement = BAS;
      if (currentLevelData[(Joueur.Y + 1) * NB_COLONNES_NIVEAUX + Joueur.X ] == '$' || currentLevelData[(Joueur.Y + 1) * NB_COLONNES_NIVEAUX + Joueur.X ] == '*') {
        if (currentLevelData[(Joueur.Y + 2) * NB_COLONNES_NIVEAUX + Joueur.X ] == ' ' || currentLevelData[(Joueur.Y + 2) * NB_COLONNES_NIVEAUX + Joueur.X ] == '.') {
          Joueur.En_mouvement = BAS;
          BougeCaisse.Y = 1;
          Joueur.Pousse = 1;
        }
      }
    }
    if (gb.buttons.pressed(BUTTON_LEFT)) {
      Joueur.Direction = GAUCHE;
      if (currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  - 1] == ' ' || currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  - 1] == '.') Joueur.En_mouvement = GAUCHE;
      if (currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  - 1] == '$' || currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  - 1] == '*') {
        if (currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  - 2] == ' ' || currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  - 2] == '.') {
          Joueur.En_mouvement = GAUCHE;
          BougeCaisse.X = -1;
          Joueur.Pousse = 1;
        }
      }
    }
    if (gb.buttons.pressed(BUTTON_RIGHT)) {
      Joueur.Direction = DROITE;
      if (currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  + 1] == ' ' || currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  + 1] == '.') Joueur.En_mouvement = DROITE;
      if (currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  + 1] == '$' || currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  + 1] == '*') {
        if (currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  + 2] == ' ' || currentLevelData[Joueur.Y * NB_COLONNES_NIVEAUX + Joueur.X  + 2] == '.') {
          Joueur.En_mouvement = DROITE;
          BougeCaisse.X = 1;
          Joueur.Pousse = 1;
        }
      }
    }
    if (Joueur.Pousse == 1) {
        soundtoplay = SoundToPlay::MoveBox;
    } else {
      if (Joueur.En_mouvement != STOP) soundtoplay = SoundToPlay::MovePlayer;
    }
  }
}
