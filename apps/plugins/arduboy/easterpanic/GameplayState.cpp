#include "GameplayState.h"

#include "Images.h"
#include "Strings.h"
#include "Utils.h"
#include "Levels.h"
#include "LevelData.h"
#include "TileLoader.h"

void GameplayState::activate(StateMachine & machine)
{
	auto selectedLevel = machine.getContext().selectedLevel;

	this->loadLevel(selectedLevel);

	this->selectedOption = 1;
	this->state = StateType::Options;
}

void GameplayState::update(StateMachine & machine)
{
	// ToDo: break this out into its own StateMachine
	switch(this->state)
	{
		case StateType::EditingActionList:
		{
			this->updateEditingActionList(machine);
			break;
		}
		case StateType::EditingCurrentAction:
		{
			this->updateEditingCurrentAction(machine);
			break;
		}
		case StateType::RunningActions:
		{
			this->updateRunningActions(machine);
			break;
		}
		case StateType::Error:
		{
			this->updateError(machine);
			break;
		}
		case StateType::Success:
		{
			this->updateSuccess(machine);
			break;
		}
		case StateType::Options:
		{
			this->updateOptions(machine);
			break;
		}
		case StateType::Quit:
		{
			machine.changeState(StateId::LevelSelect);
			break;
		}
		default:
		{
			break;
		}
	}
}

void GameplayState::render(StateMachine & machine)
{
	this->renderPlayfield(machine);

	// ToDo: break this out into its own StateMachine
	switch(this->state)
	{
		case StateType::EditingActionList:
		{
			this->renderEditingActionList(machine);
			break;
		}
		case StateType::EditingCurrentAction:
		{
			this->renderEditingCurrentAction(machine);
			break;
		}
		case StateType::RunningActions:
		{
			this->renderRunningActions(machine);
			break;
		}
		case StateType::Error:
		{
			this->renderError(machine);
			break;
		}
		case StateType::Success:
		{
			this->renderSuccess(machine);
			break;
		}
		case StateType::Options:
		{
			this->renderOptions(machine);
			break;
		}
		case StateType::Quit:
		{
			break;
		}
		default:
		{
			break;
		}
	}
}

//
// State::Error
//

void GameplayState::updateError(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;

	if (arduboy.justPressed(Arduboy::ButtonA))
	{
		this->state = StateType::EditingActionList;
	}
}

const char * GameplayState::getErrorStringLine0(ErrorType type)
{
	return ProgmemRead<const char *>(&ErrorStringsLine0[static_cast<uint8_t>(type)]);
}

const char * GameplayState::getErrorStringLine1(ErrorType type)
{
	return ProgmemRead<const char *>(&ErrorStringsLine1[static_cast<uint8_t>(type)]);
}

void GameplayState::renderError(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;

	auto errorLine0 = getErrorStringLine0(this->error);
	auto errorLine1 = getErrorStringLine1(this->error);

	auto errorString0Width = ProgmemStringWidth(errorLine0);
	auto errorString1Width = ProgmemStringWidth(errorLine1);

	auto maxWidth = max(errorString0Width, errorString1Width);

	auto messageY = CalculateCentreY(StringHeight(3));

	{
		auto rectX = CalculateCentreX(maxWidth) - 2;
		auto rectY = messageY - 2;
		auto rectWidth = maxWidth + 4;
		auto rectHeight = StringHeight(3) + 4;

		arduboy.fillRect(rectX, rectY, rectWidth, rectHeight, Arduboy::ColourBlack);
		arduboy.drawRect(rectX, rectY, rectWidth, rectHeight, Arduboy::ColourWhite);
	}

	{
		auto messageX = CalculateCentreX(StringWidth(StringErrorHeading));
		arduboy.setCursor(messageX, messageY);
		arduboy.print(FlashString(StringErrorHeading));
		messageY += FontLineHeight;
	}
	{
		auto messageX = CalculateCentreX(errorString0Width);
		arduboy.setCursor(messageX, messageY);
		arduboy.print(FlashString(errorLine0));
		messageY += FontLineHeight;
	}
	{
		auto messageX = CalculateCentreX(errorString1Width);
		arduboy.setCursor(messageX, messageY);
		arduboy.print(FlashString(errorLine1));
		messageY += FontLineHeight;
	}
}

//
// State::Success
//

void GameplayState::updateSuccess(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;

	if (arduboy.justPressed(Arduboy::ButtonA))
	{
		machine.changeState(StateId::LevelSelect);
	}
}

void GameplayState::renderSuccess(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;

	auto messageY = CalculateCentreY(StringHeight(1));
	auto messageX = CalculateCentreX(StringWidth(StringSuccessHeading));
	arduboy.setCursor(messageX, messageY);
	arduboy.print(FlashString(StringErrorHeading));
	{
		auto rectX = messageX - 2;
		auto rectY = messageY - 2;
		auto rectWidth = StringWidth(StringSuccessHeading) + 4;
		auto rectHeight = StringHeight(1) + 4;

		arduboy.fillRect(rectX, rectY, rectWidth, rectHeight, Arduboy::ColourBlack);
		arduboy.drawRect(rectX, rectY, rectWidth, rectHeight, Arduboy::ColourWhite);
		arduboy.setCursor(messageX, messageY);
		arduboy.print(FlashString(StringSuccessHeading));
	}
}

//
// State::Options
//

const GameplayState::MenuOption GameplayState::menuOptions[] PROGMEM =
{
	MenuOption(StateType::RunningActions, 1, StringRun),
	MenuOption(StateType::EditingActionList, 2, StringEdit),
	MenuOption(StateType::Quit, 0, StringExit),
};

void GameplayState::updateOptions(StateMachine & machine)
{
	auto & arduboy = machine.getContext().arduboy;

	using MenuOptionRange = StaticRange<uint8_t, ArrayFirstIndex(menuOptions), ArrayLastIndex(menuOptions)>;
	
	if (arduboy.justPressed(Arduboy::ButtonLeft))
	{
		this->selectedOption = MenuOptionRange::wrappedPrevious(this->selectedOption);
	}

	if (arduboy.justPressed(Arduboy::ButtonRight))
	{
		this->selectedOption = MenuOptionRange::wrappedNext(this->selectedOption);
	}

	if (arduboy.justPressed(Arduboy::ButtonA))
	{
		this->state = ProgmemRead<StateType>(&this->menuOptions[this->selectedOption].state);
		if(this->state == StateType::RunningActions)
		{
			this->nextAction = 0;
			const auto selectedLevel = machine.getContext().selectedLevel;
			this->loadLevel(selectedLevel);
		}
	}
}

void GameplayState::renderOptions(StateMachine & machine)
{
	// Layout constants
	constexpr const uint8_t singleMargin = 2;
	constexpr const uint8_t doubleMargin = singleMargin * 2;
	
	constexpr const uint8_t selectedIconWidth = MenuIconWidth + doubleMargin;
	constexpr const uint8_t selectedIconHeight = MenuIconHeight + doubleMargin;
	
	constexpr const uint8_t centreX = HalfScreenWidth + CalculateCentre(HalfScreenWidth, ActionIconWidth);
	constexpr const uint8_t centreY = CalculateCentreY(ActionIconHeight);

	// Draw options	
	{
		using MenuOptionRange = StaticRange<uint8_t, ArrayFirstIndex(menuOptions), ArrayLastIndex(menuOptions)>;
		
		const auto index1 = MenuOptionRange::wrappedNext(this->selectedOption);
		const auto index2 = MenuOptionRange::wrappedPrevious(this->selectedOption);
		
		const auto optionImageIndex1 = ProgmemRead(&menuOptions[index1].imageIndex);
		const auto optionImageIndex2 = ProgmemRead(&menuOptions[index2].imageIndex);
		
		Sprites::drawOverwrite(centreX + selectedIconWidth, centreY, MenuIcons, optionImageIndex1);
		Sprites::drawOverwrite(centreX - selectedIconWidth, centreY, MenuIcons, optionImageIndex2);
		Sprites::drawSelfMasked(centreX + selectedIconWidth -1, centreY - 1, OutlineBox, 0);
		Sprites::drawSelfMasked(centreX - selectedIconWidth - 1, centreY - 1, OutlineBox, 0);
	}
	
	// Draw selected option
	const auto selectedOption = ProgmemRead(&menuOptions[this->selectedOption]);
	Sprites::drawOverwrite(centreX, centreY, MenuIcons, selectedOption.imageIndex);
	
	// Get Arduboy reference
	auto & arduboy = machine.getContext().arduboy;

	// Print option name
	{
		const auto optionWidth = ProgmemStringWidth(selectedOption.text);
		const auto textX = HalfScreenWidth + CalculateCentre(HalfScreenWidth, optionWidth);
		const auto textY = ScreenHeight - (FontLineHeight + singleMargin);
			
		arduboy.setCursor(textX, textY);		
		arduboy.print(FlashString(selectedOption.text));
	}
	
	// Print level number
	{
		const auto selectedLevel = machine.getContext().selectedLevel;
		const auto digitCount = countDigitsConst(selectedLevel);
		const auto digitWidth = StringWidth(digitCount);
		const auto stringWidth = StringWidth(StringLevelHeading) + digitWidth;
		const auto levelX = HalfScreenWidth + CalculateCentre(HalfScreenWidth, stringWidth);
		arduboy.setCursor(levelX, singleMargin);
		arduboy.print(FlashString(StringLevelHeading));
		arduboy.print(selectedLevel);
	}

	// Grouping the drawRect calls reduces the code size somewhat
	
	// Draw selector
	arduboy.drawRect(centreX - singleMargin, centreY - singleMargin, selectedIconWidth, selectedIconHeight, Arduboy::ColourWhite);
	
	// Draw box around option name
	arduboy.drawRect(HalfScreenWidth, ScreenHeight - (FontLineHeight + doubleMargin), HalfScreenWidth, FontLineHeight + doubleMargin, Arduboy::ColourWhite);
	
	// Draw box around level number
	arduboy.drawRect(HalfScreenWidth, 0, HalfScreenWidth, FontLineHeight + doubleMargin, Arduboy::ColourWhite);
	
	// Draw border
	arduboy.drawRect(HalfScreenWidth, 0, HalfScreenWidth, ScreenHeight, Arduboy::ColourWhite);
}

//
// State::EditingActionList
//

void GameplayState::updateEditingActionList(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;

	if(arduboy.justPressed(Arduboy::ButtonLeft))
	{
		if(this->selectedAction > this->actions.getFirstIndex())
			--this->selectedAction;
	}

	if(arduboy.justPressed(Arduboy::ButtonRight))
	{
		if(this->selectedAction < this->actions.getLastIndex())
			++this->selectedAction;
	}

	if(arduboy.justPressed(Arduboy::ButtonA))
	{
		this->state = StateType::EditingCurrentAction;
	}

	if(arduboy.justPressed(Arduboy::ButtonB))
	{
		this->state = StateType::Options;
	}
}

void GameplayState::renderEditingActionList(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;

	auto x = HalfScreenWidth + 4;
	auto y = 2;

	for(uint8_t i = 0; i < this->actions.getCount(); ++i)
	{
		auto divi = i / 7;
		auto modi = i % 7;

		auto drawX = x + (modi * 8);
		auto drawY = y + (divi * 8);

		arduboy.drawRect(drawX, drawY, 8, 8, Arduboy::ColourWhite);

		auto colour = (this->actions[i].getId() != ActionId::None) ? Arduboy::ColourWhite : Arduboy::ColourBlack;
		arduboy.fillRect(drawX + 2, drawY + 2, 4, 4, colour);

		if(i == this->selectedAction)
			arduboy.drawRect(drawX - 2, drawY - 2, 12, 12, Arduboy::ColourWhite);
	}
}

//
// State::EditingCurrentAction
//

void GameplayState::updateEditingCurrentAction(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;

	auto & action = this->actions[this->selectedAction];

	if(arduboy.justPressed(Arduboy::ButtonUp))
	{
		if(this->editingArgument)
		{
			if(action.getArgument() < 255)
				++action.getArgument();
		}
		else
			action.getId() = previousActionIdWrapped(action.getId());
	}

	if(arduboy.justPressed(Arduboy::ButtonDown))
	{
		if(this->editingArgument)
		{
			if(action.getArgument() > 0)
				--action.getArgument();
		}
		else
			action.getId() = nextActionIdWrapped(action.getId());
	}

	if(arduboy.justPressed(Arduboy::ButtonRight))
	{
		if(action.getId() == ActionId::ForStart)
			this->editingArgument = true;
	}

	if(arduboy.justPressed(Arduboy::ButtonLeft))
	{
		this->editingArgument = false;
	}

	if(arduboy.justPressed(Arduboy::ButtonA))
	{
		this->state = StateType::EditingActionList;
	}
}

void GameplayState::renderEditingCurrentAction(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;
	{
		auto x = HalfScreenWidth + 4;
		auto y = 2;

		for(uint8_t i = 0; i < this->actions.getCount(); ++i)
		{
			auto divi = i / 7;
			auto modi = i % 7;

			auto drawX = x + (modi * 8);
			auto drawY = y + (divi * 8);

			arduboy.drawRect(drawX, drawY, 8, 8, Arduboy::ColourWhite);

			auto colour = (this->actions[i].getId() != ActionId::None) ? Arduboy::ColourWhite : Arduboy::ColourBlack;
			arduboy.fillRect(drawX + 2, drawY + 2, 4, 4, colour);

			if(i == this->selectedAction)
				arduboy.drawRect(drawX - 2, drawY - 2, 12, 12, Arduboy::ColourWhite);
		}
	}
	{
		constexpr const uint8_t x = CalculateCentreX(HalfScreenWidth, ScreenWidth, ActionIconWidth);
		constexpr const uint8_t y = ScreenHeight - (ActionIconHeight + 2);
		auto & action = this->actions[this->selectedAction];
		Sprites::drawOverwrite(x, y, ActionIcons, static_cast<uint8_t>(action.getId()));
		if(action.getId() == ActionId::ForStart)
		{
			arduboy.setCursor(x + ActionIconWidth + 2, y + ((ActionIconHeight - FontLineHeight) / 2));
			arduboy.print(action.getArgument());

			auto arrowX = this->editingArgument ? x + ActionIconWidth + 2 : x + 2;
			Sprites::drawOverwrite(arrowX, y - SmallArrowImageHeight, SmallArrowImages, 2);
		}
	}
}

//
// State::RunningActions
//

void GameplayState::updateRunningActions(StateMachine & machine)
{
	if(this->nextAction >= this->actions.getCount())
		return;

	auto arduboy = machine.getContext().arduboy;

	if(arduboy.justPressed(Arduboy::ButtonB))
	{
		this->state = StateType::Options;
	}

	if((arduboy.pressed(Arduboy::ButtonB) && arduboy.everyXFrames(30)) || arduboy.everyXFrames(60))
	{
		auto action = this->actions[this->nextAction];
		switch(action.getId())
		{
			case ActionId::None: break;
			case ActionId::MoveForward:
			{
				switch(this->player.direction)
				{
					case Direction::North:
					{
						if(this->player.y > 0)
						{
							auto next = this->player.y - 1;
							if(!this->tiles.getItem(this->player.x, next).isSolid())
								this->player.y = next;
						}
						break;
					}
					case Direction::East:
					{
						if(this->player.x < (this->tiles.getWidth() - 1))
						{
							auto next = this->player.x + 1;
							if(!this->tiles.getItem(next, this->player.y).isSolid())
								this->player.x = next;
						}
						break;
					}
					case Direction::South:
					{
						if(this->player.y < (this->tiles.getHeight() - 1))
						{
							auto next = this->player.y + 1;
							if(!this->tiles.getItem(this->player.x, next).isSolid())
								this->player.y = next;
						}
						break;
					}
					case Direction::West:
					{
						if(this->player.x > 0)
						{
							auto next = this->player.x - 1;
							if(!this->tiles.getItem(next, this->player.y).isSolid())
								this->player.x = next;
						}
						break;
					}
					default:
					{
						this->reportError(ErrorType::InvalidPlayerDirection);
						return;
					}
				}
				this->checkForCollectableCollision();
				if(this->collectables.isEmpty())
				{
					this->state = StateType::Success;
					return;
				}
				break;
			}
			case ActionId::TurnLeft:
			{
				--this->player.direction;
				break;
			}
			case ActionId::TurnRight:
			{
				++this->player.direction;
				break;
			}
			case ActionId::ForStart:
			{
				if(this->forStack.isFull())
				{
					// This might happen but is unlikely
					this->reportError(ErrorType::TooManyNestedForLoops);
					return;
				}

				this->forStack.push(ForLoopState(action.getArgument(), this->nextAction));
				break;
			}
			case ActionId::ForEnd:
			{
				if(this->forStack.isEmpty())
				{
					// This should never happen
					this->reportError(ErrorType::TooManyForEnds);
					return;
				}

				auto & state = this->forStack.peek();
				if(state.next())
					nextAction = state.getActionIndex();
				else
					this->forStack.drop();
				break;
			}
			default:
			{
				this->reportError(ErrorType::UnrecognisedInstruction);
				return;
			}
		}

		++this->nextAction;

		if(this->nextAction >= this->actions.getCount())
		{
			this->state = StateType::EditingActionList;
			if(!this->forStack.isEmpty())
			{
				// This should never happen
				this->reportError(ErrorType::TooManyForStarts);
				return;
			}
		}
	}
}

void GameplayState::renderRunningActions(StateMachine & machine)
{
	constexpr const uint8_t x = CalculateCentreX(HalfScreenWidth, ScreenWidth, ActionIconWidth);
	constexpr const uint8_t y = ScreenHeight - (ActionIconHeight + 2);

	if(this->nextAction > 0)
	{
		auto id = this->actions[this->nextAction - 1].getId();
		Sprites::drawOverwrite(x, y, ActionIcons, static_cast<uint8_t>(id));
	}

	(void)machine; // Remove warning
}

//
// Misc
//


void GameplayState::loadLevel(uint8_t selectedLevel)
{
	if(selectedLevel > ArrayLastIndex(Levels))
	{
		// Panic!!
		return;
	}

	LevelData levelData = ProgmemRead(&Levels[selectedLevel]);

	this->actions.resize(levelData.maxActions);
	this->player = levelData.playerStart;
	TileLoader<TileGrid::Width, TileGrid::Height>::LoadProgmem(this->tiles, levelData.tileData);

	this->collectables.clear();
	auto count = levelData.collectableData.collectableCount;
	for(uint8_t i = 0; i < count; ++i)
	{
		this->collectables.add(ProgmemRead(&levelData.collectableData.collectables[i]));
	}
}

void GameplayState::reportError(ErrorType error)
{
	this->error = error;
	this->state = StateType::Error;
}

void GameplayState::checkForCollectableCollision(void)
{
	for(uint8_t i = this->collectables.getFirstIndex(); i < this->collectables.getCount(); ++i)
	{
		if(this->player.x == this->collectables[i].x && this->player.y == this->collectables[i].y)
		{
			this->collectables.removeAt(i);
			break;
		}
	}
}

//
// Misc Render
//

void GameplayState::renderGrid(StateMachine & machine)
{
	auto arduboy = machine.getContext().arduboy;

	const uint8_t tileWidth = SmallRabbitImageWidth;
	const uint8_t tileHeight = SmallRabbitImageHeight;

	for(uint8_t y = 0; y < TileGrid::Height; ++y)
	for(uint8_t x = 0; x < TileGrid::Width; ++x)
	{
		const auto tile = tiles.getItem(x, y);
		if(tile.isSolid())
			arduboy.drawRect(x * tileWidth, y * tileHeight, tileWidth, tileHeight, Arduboy::ColourWhite);
	}
}

void GameplayState::renderEntity(StateMachine & machine)
{
	Sprites::drawOverwrite(player.x * 8, player.y * 8, SmallRabbitImages, static_cast<uint8_t>(player.direction));

	(void)machine; // Remove warning
}

void GameplayState::renderCollectables(StateMachine & machine)
{
	for(uint8_t i = 0; i < collectables.getCount(); ++i)
	{
		Sprites::drawOverwrite(collectables[i].x * 8, collectables[i].y * 8, SmallCollectableImages, 0);
	}

	(void)machine; // Remove warning
}

void GameplayState::renderPlayfield(StateMachine & machine)
{
	renderGrid(machine);
	renderCollectables(machine);
	renderEntity(machine);
}