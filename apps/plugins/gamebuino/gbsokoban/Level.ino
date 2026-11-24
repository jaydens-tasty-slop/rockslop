void count_Boxes() {
  Boxes = 0;
  for (int ligne = 0; ligne < NB_LIGNES_NIVEAUX; ligne++) {
    for (int colonne = 0; colonne < NB_COLONNES_NIVEAUX; colonne++) {
      if (currentLevelData[ligne * NB_COLONNES_NIVEAUX + colonne] == '$') {
        ++Boxes;
      } 
    }
  }
};

void count_Boxes_On_Good_Place() {
  PlacedBoxes = 0;
  for (int ligne = 0; ligne < NB_LIGNES_NIVEAUX; ligne++) {
    for (int colonne = 0; colonne < NB_COLONNES_NIVEAUX; colonne++) {
      if (currentLevelData[ligne * NB_COLONNES_NIVEAUX + colonne] == '*') {
        ++PlacedBoxes;
      } 
    }
  }
};
