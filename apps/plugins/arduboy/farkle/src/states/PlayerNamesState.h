#pragma once

#include "../utils/GameContext.h"
#include "../utils/GameState.h"
#include "../utils/Enums.h"
#include "../utils/Name.h"
#include "../images/Images.h"
#include "BaseState.h"

class PlayerNamesState : public BaseState {

  private:

    enum class ViewState : uint8_t {
      NumberOfPlayers,
      EnterName,
    };

    ViewState viewState = ViewState::NumberOfPlayers;
    uint16_t frameCount = 0;
    Name name;

  public:	
  
    void activate(StateMachine & machine) override;
    void update(StateMachine & machine) override;
    void render(StateMachine & machine) override;

    void getPlayerName(StateMachine & machine);
    void updatePlayerName(StateMachine & machine);
    void setNumberOfPlayers(StateMachine & machine, uint8_t numberOfPlayers);

};

