#pragma once

//
// Titlescreen strings
//

char const StringPlay[] PROGMEM = "Play";
char const StringCredits[] PROGMEM = "Credits";
char const StringStats[] PROGMEM = "Stats";

//
// Level select strings
//

char const StringLevelHeading[] PROGMEM = "Level ";

//
// Gamplay other strings
//

char const StringSuccessHeading[] PROGMEM = "Success!";


char const StringEdit[] PROGMEM = "Edit";
char const StringRun[] PROGMEM = "Run";
char const StringExit[] PROGMEM = "Exit";

char const * const StringOptions[] PROGMEM =
{
	StringEdit,
	StringRun,
	StringExit,
};


//
// Gameplay Error strings
//

char const StringErrorHeading[] PROGMEM = "Error:";

char const StringErrorNoneLine0[] PROGMEM = "No errors";
char const StringErrorNoneLine1[] PROGMEM = "detected";

char const StringErrorUnrecognisedErrorLine0[] PROGMEM = "Unrecognised";
char const StringErrorUnrecognisedErrorLine1[] PROGMEM = "error";

char const StringErrorUnrecognisedInstructionLine0[] PROGMEM = "Unrecognised";
char const StringErrorUnrecognisedInstructionLine1[] PROGMEM = "instruction";

char const StringErrorTooManyForStartsLine0[] PROGMEM = "Too many";
char const StringErrorTooManyForStartsLine1[] PROGMEM = "fors";

char const StringErrorTooManyForEndsLine0[] PROGMEM = "Too many";
char const StringErrorTooManyForEndsLine1[] PROGMEM = "ends";

char const StringErrorTooManyNestedForLoopsLine0[] PROGMEM = "Too many";
char const StringErrorTooManyNestedForLoopsLine1[] PROGMEM = "nested loops";

char const StringErrorInvalidPlayerDirectionLine0[] PROGMEM = "Invalid";
char const StringErrorInvalidPlayerDirectionLine1[] PROGMEM = "player direction";

char const * const ErrorStringsLine0[] PROGMEM =
{
	StringErrorNoneLine0,
	StringErrorUnrecognisedErrorLine0,
	StringErrorUnrecognisedInstructionLine0,
	StringErrorTooManyForStartsLine0,
	StringErrorTooManyForEndsLine0,
	StringErrorTooManyNestedForLoopsLine0,
	StringErrorInvalidPlayerDirectionLine0,
};

char const * const ErrorStringsLine1[] PROGMEM =
{
	StringErrorNoneLine1,
	StringErrorUnrecognisedErrorLine1,
	StringErrorUnrecognisedInstructionLine1,
	StringErrorTooManyForStartsLine1,
	StringErrorTooManyForEndsLine1,
	StringErrorTooManyNestedForLoopsLine1,
	StringErrorInvalidPlayerDirectionLine1,
};