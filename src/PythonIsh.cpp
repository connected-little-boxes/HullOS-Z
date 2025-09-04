#include <Arduino.h>
#include "HullOSVariables.h"
#include "HullOSCommands.h"
#include "Utils.h"
#include "HullOSScript.h"
#include "PythonIsh.h"
#include "console.h"
#include "HullOS.h"
#include "messages.h"

int pythonIshdecodeScriptLine(char *input);

void pythonIshDecoderStart()
{
	Serial.printf("Starting PythonIsh decoder\n");
}

struct LanguageHandler PythonIshLanguage = {
	"PythonIsh",
	pythonIshDecoderStart,
	pythonIshdecodeScriptLine,
	pythonIshShowPrompt
};

void pythonIshShowPrompt()
{
	if (storingProgram()){
		alwaysDisplayMessage("P*>");
	}
	else {
		alwaysDisplayMessage("P>");
	}
}

const char pythonishcommandNames[] =
	"angry#"	  // COMMAND_ANGRY       0
	"happy#"	  // COMMAND_HAPPY       1
	"move#"		  // COMMAND_MOVE        2
	"turn#"		  // COMMAND_TURN        3
	"arc#"		  // COMMAND_ARC         4
	"delay#"	  // COMMAND_DELAY       5
	"colour#"	  // COMMAND_COLOUR      6
	"color#"	  // COMMAND_COLOR       7
	"pixel#"	  // COMMAND_PIXEL       8
	"set#"		  // COMMAND_SET         9
	"if#"		  // COMMAND_IF         10
	"do#"		  // COMMAND_DO         11
	"while#"	  // COMMAND_WHILE      12
	"intime#"	  // COMMAND_INTIME     13
	"endif#"	  // COMMAND_ENDIF      14
	"forever#"	  // COMMAND_FOREVER    15
	"endwhile#"	  // COMMAND_ENDWHILE   16
	"sound#"	  // COMMAND_SOUND      17
	"until#"	  // COMMAND_UNTIL      18
	"clear#"	  // COMMAND_CLEAR      19
	"run#"		  // COMMAND_RUN        20
	"background#" // COMMAND_BACKGROUND 21
	"else#"		  // COMMAND_ELSE       22
	"red#"		  // COMMAND_RED        23
	"green#"	  // COMMAND_GREEN      24
	"blue#"		  // COMMAND_BLUE       25
	"yellow#"	  // COMMAND_YELLOW     26
	"magenta#"	  // COMMAND_MAGENTA    27
	"cyan#"		  // COMMAND_CYAN       28
	"white#"	  // COMMAND_WHITE      29
	"black#"	  // COMMAND_BLACK      30
	"wait#"       // COMMAND_WAIT       31
	"stop#"		  // COMMAND_STOP       32
	"begin#"	  // COMMAND_BEGIN      33
	"end#"		  // COMMAND_END        34
	"print#"	  // COMMAND_PRINT      35
	"println#"	  // COMMAND_PRINTLN    36
	"break#"	  // COMMAND_BREAK      37
	"duration#"	  // COMMAND_DURATION   38
	"continue#"	  // COMMAND_CONTINUE   39
	"angle#"	  // COMMAND_ANGLE      40
	"save#"		  // COMMAND_SAVE       41
	"load#"		  // COMMAND_LOAD       42
	"dump#"		  // COMMAND_DUMP       43
	"chain#"	  // COMMAND_CHAIN      44
	"send#"	      // COMMAND_SEND       45
	"nowait#"     // COMMAND_NO_WAIT    46
	;


const char completeAwaitCommand[] = "CA";

int handleInTime()
{
	bool wantWait=true;

	while (*bufferPos != 0)
	{

		skipInputSpaces(); // find the next character

		// Spin further down the commands looking for an intime command
		int command = decodeCommandName(pythonishcommandNames);

		if (command == COMMAND_NO_WAIT){
			if(*bufferPos){
				return ERROR_NO_WAIT_SHOULD_BE_THE_LAST_THING_ON_A_LINE;
			}
			else {
				wantWait=false;
			}
			break;
		}

		if (command == COMMAND_INTIME) // 13 is the offset in the command names of the intime word
		{
			HullOSProgramoutputFunction(',');

			skipInputSpaces();

			if (*bufferPos == 0)
				return ERROR_MISSING_TIME_VALUE_IN_INTIME; // missing time number

			int result = processValue();

			if (result != ERROR_OK)
				return result;
		}
		else
		{
			return ERROR_INTIME_EXPECTED_BUT_NOT_SUPPLIED;
		}
	}

	if(wantWait){
		endCommand(); // end the movement command
		sendCommand(completeAwaitCommand);
	}

	previousStatementStartedBlock = false;

	return ERROR_OK;
}

// The value to be supplied gives a limit of some kind, either distance or angle
// If the value is missed off the program provides a large default value and the
// the program does not pause during the move. If the distance value is given this
// will cause the program to pause until the distance has been travelled

const char largeLimitValue[] = "20000";

int handleValueIntimeAndBackground()
{
	if (*bufferPos == 0)
	{
		// No value, that's fine - just out the large limit
		sendCommand(largeLimitValue);
		return ERROR_OK;
	}
	else
	{
		// have a value - process it
		skipInputSpaces();

		int result = processValue();

		if (result != ERROR_OK)
			return result;
	}

	return handleInTime();
}

const char moveCommand[] = "MF";

int compileMove()
{
#ifdef SCRIPT_DEBUG
	Serial.println(F("Compiling move: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a move
	previousStatementStartedBlock = false;

	sendCommand(moveCommand);

	skipInputSpaces();

	return handleValueIntimeAndBackground();
}

const char turnCommand[] = "MR";

int compileTurn()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling turn: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a turn
	previousStatementStartedBlock = false;

	skipInputSpaces();

	sendCommand(turnCommand);

	return handleValueIntimeAndBackground();
}

const char arcCommand[] = "MA";

int compileArc()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling arc: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after an arc
	previousStatementStartedBlock = false;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_NO_RADIUS_IN_ARC;
	}

	sendCommand(arcCommand);

	int result = processValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	// Spin further down the commands looking for an intime command
	int command = decodeCommandName(pythonishcommandNames);

	if (command != COMMAND_ANGLE)
	{
		return ERROR_NO_ANGLE_IN_ARC;
	}

	HullOSProgramoutputFunction(',');

	skipInputSpaces();

	return handleValueIntimeAndBackground();
}

const char delayCommand[] = "CD";

int compileDelay()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling delay: "));
#endif // SCRIPT_DEBUG

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_TIME_IN_DELAY;
	}

	sendCommand(delayCommand);

	previousStatementStartedBlock = false;

	return processValue();
}

const char colourCommand[] = "PC";

int compileColour()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling colour: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a sound
	previousStatementStartedBlock = false;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_RED_VALUE_IN_COLOUR;
	}

	sendCommand(colourCommand);

	int result = processValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_GREEN_VALUE_IN_COLOUR;
	}

	if (*bufferPos != ',')
	{
		return ERROR_MISSING_GREEN_VALUE_IN_COLOUR;
	}

	HullOSProgramoutputFunction(',');

	bufferPos++;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_GREEN_VALUE_IN_COLOUR;
	}

	result = processValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_BLUE_VALUE_IN_COLOUR;
	}

	if (*bufferPos != ',')
	{
		return ERROR_MISSING_BLUE_VALUE_IN_COLOUR;
	}

	HullOSProgramoutputFunction(',');

	bufferPos++;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_BLUE_VALUE_IN_COLOUR;
	}

	return processValue();
}

const char namedColourCommand[] = "PN";

int compileSimpleColor()
{
	sendCommand(namedColourCommand);

	// Send the first character of the colour name
	HullOSProgramoutputFunction(*commandStartPos);

	previousStatementStartedBlock = false;

	return ERROR_OK;
}

int compileBlack()
{
	sendCommand(namedColourCommand);

	// Send the black colour name
	HullOSProgramoutputFunction('k');

	previousStatementStartedBlock = false;

	return ERROR_OK;
}

int compilePixel()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling pixel: "));
#endif // SCRIPT_DEBUG

	return ERROR_NOT_IMPLEMENTED;
}

const char soundCommand[] = "ST";
const char defaultSoundDuration[] = "500";

int compileSound()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling sound: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a sound
	previousStatementStartedBlock = false;

	skipInputSpaces();

	if (*bufferPos == 0)
	{
		return ERROR_MISSING_PITCH_VALUE_IN_SOUND;
	}

	sendCommand(soundCommand);

	int result = processValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	HullOSProgramoutputFunction(',');

	bool doWait = true;

	if (*bufferPos == 0)
	{
		// no duration or wait - use default duration
		sendCommand(defaultSoundDuration);
	}
	else
	{
		// Spin further down the commands looking for duration or wait

		int command = decodeCommandName(pythonishcommandNames);

		if (command == COMMAND_NO_WAIT)
		{
			// send the default duration
			sendCommand(defaultSoundDuration);
			// continue running while the sound is playing
			doWait = false;
		}
		else
		{
			if (command == COMMAND_DURATION)
			{
				skipInputSpaces();
				result = processValue();
				if (result != ERROR_OK)
					return result;
			}
			else
			{
				return ERROR_SECOND_COMMAND_IN_SOUND_IS_NOT_DURATION;
			}

			skipInputSpaces();

			if (*bufferPos != 0)
			{
				// Now look for a wait
				command = decodeCommandName(pythonishcommandNames);

				if (command == COMMAND_NO_WAIT)
				{
					doWait = false;
				}
				else
				{
					return ERROR_INVALID_COMMAND_AFTER_DURATION_SHOULD_BE_NO_WAIT;
				}
			}
		}
	}

	HullOSProgramoutputFunction(',');

	if (doWait)
		HullOSProgramoutputFunction('W');
	else
		HullOSProgramoutputFunction('N');

	return ERROR_OK;
}

const char setCommand[] = "VS";

int compileAssignment()
{
#ifdef SCRIPT_DEBUG
	Serial.print(F("Compiling set: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a set
	previousStatementStartedBlock = false;

	skipInputSpaces();

	if (checkIdentifier(bufferPos) != VARIABLE_NAME_OK)
		return ERROR_INVALID_VARIABLE_NAME_IN_SET;

	int position;

	if (findVariable(bufferPos, &position) == VARIABLE_NOT_FOUND)
	{
		if (createVariable(bufferPos, &position) == NO_ROOM_FOR_VARIABLE)
		{
			return ERROR_TOO_MANY_VARIABLES;
		}
	}

	sendCommand(setCommand);

	writeBytesFromBuffer(getVariableNameLength(position));

	skipInputSpaces();

	if (*bufferPos != '=')
	{
		return ERROR_NO_EQUALS_IN_SET;
	}

	bufferPos++;					  // skip past the equals
	HullOSProgramoutputFunction('='); // write the equals

	skipInputSpaces();

	return processValue();
}

int processCommand(byte commandNo)
{
	switch (commandNo)
	{
	case COMMAND_ANGRY: // angry
		return compileAngry();

	case COMMAND_HAPPY: // happy
		return compileHappy();

	case COMMAND_MOVE: // move
		return compileMove();

	case COMMAND_TURN: // turn
		return compileTurn();

	case COMMAND_ARC: // arc
		return compileArc();

	case COMMAND_DELAY: // delay
		return compileDelay();

	case COMMAND_COLOUR: // colour
		return compileColour();

	case COMMAND_COLOR: // color
		return compileColour();

	case COMMAND_PIXEL: // pixel
		return compilePixel();

	case COMMAND_IF: // if
		return compileIf();

	case COMMAND_WHILE: // while
		return compileWhile();

	case COMMAND_CLEAR: // clear
		return clearProgram();

	case COMMAND_RUN: // run
		return runProgram();

	case COMMAND_ELSE: // else
		return compileElse();

	case COMMAND_FOREVER: // forever
		return compileForever();

	case COMMAND_SET:
		return compileAssignment();

	case COMMAND_RED:
	case COMMAND_BLUE:
	case COMMAND_GREEN:
	case COMMAND_MAGENTA:
	case COMMAND_CYAN:
	case COMMAND_YELLOW:
	case COMMAND_WHITE:
		return compileSimpleColor();

	case COMMAND_BLACK:
		return compileBlack();

	case COMMAND_WAIT:
		return compileWait();

	case COMMAND_STOP:
		return compileStop();

	case COMMAND_BEGIN:
		return compileBegin();

	case COMMAND_END:
		return compileEnd();

	case COMMAND_PRINT:
		return compilePrint();

	case COMMAND_SEND:
		return compileSend();

	case COMMAND_PRINTLN:
		return compilePrintln();

	case COMMAND_SYSTEM_COMMAND:
		return compileDirectCommand();

	case COMMAND_SOUND:
		return compileSound();

	case COMMAND_BREAK:
		return compileBreak();

	case COMMAND_CONTINUE:
		return compileContinue();

	case COMMAND_SAVE:
		return compileProgramSave();

	case COMMAND_LOAD:
		return compileProgramLoad(true);

	case COMMAND_CHAIN:
		return compileProgramLoad(false);

	case COMMAND_DUMP:
		return compileProgramDump();

	default:
		return compileAssignment();
	}

	return ERROR_INVALID_COMMAND;
}

// Called when the indent level of a new line is less than that of the
// previous line. This means that we are closing off one or more indent
// levels. Also called at the end of compilation to close off any loops
// or conditions. Supplied with the new level of indent and the
// number of the command on the statement being indented

// #define SCRIPT_DEBUG_INDENT_OUT

int indentOutToNewIndentLevel(byte indent, int commandNo)
{
	int result;
	int labelNo;

#ifdef SCRIPT_DEBUG_INDENT_OUT
	Serial.println("Indent out to new indent level");
	Serial.print("Indent: ");
	Serial.print(indent);
	Serial.print(" Command: ");
	Serial.print(commandNo);
	Serial.print(" Current Indent Level: ");
	Serial.println(currentIndentLevel);
#endif

	while (indent < currentIndentLevel)
	{
#ifdef SCRIPT_DEBUG_INDENT_OUT
		Serial.println("Looping");
#endif
		if (operation_stack_empty())
		{
#ifdef SCRIPT_DEBUG_INDENT_OUT
			Serial.println("Operation stack empty");
#endif
			// This should not happen as it is not possible to
			// indent code without creating an enclosing command
			return ERROR_INDENT_OUTWARDS_WITHOUT_ENCLOSING_COMMAND;
		}

		// pull back the indent level to the previous one
		currentIndentLevel = top_operation_indent_level();

		// if this indent level is not the same as the indent
		// level of the item on the top of the stack we just close
		// this element off

#ifdef SCRIPT_DEBUG_INDENT_OUT
		Serial.print("New Current Indent Level: ");
		Serial.println(currentIndentLevel);
#endif
		// Generate the code to match the end of the
		// enclosing statement

		switch (top_operation_type())
		{
		case IF_CONSTRUCTION_STACK_ITEM:
#ifdef SCRIPT_DEBUG_INDENT_OUT
			Serial.print("Handling an if..");
#endif
			// might have an else clause for an if construction

			// need to spin up through the operation stack looking for
			// the if condition with the same indent as this else, as that is the
			// one that matches. Any other items that we find (including do) will
			// need to be closed off at this point

			if (currentIndentLevel == indent &&
				commandNo == COMMAND_ELSE)
			{
#ifdef SCRIPT_DEBUG_INDENT_OUT
				Serial.print("...with an else");
#endif

				// get the destination label for the end of the code
				// skipped past if the if condition is not obeyed
				// This code will be obeyed as the else clause

				// get the label number for the label reached if we jump
				// past the code controlled by the if

				labelNo = pop_operation_count();

				// drop a jump to the next label number
				// this number was reserved when the if was created
				// this is the position which will mark the end of the
				// code performed by the else - when we see the endif

				dropJumpCommand(labelNo + 1);

				// Now drop a label to serve as the destination of the
				// jump past the if clause code. This is the code obeyed
				// if else is the case.

				dropLabel(labelNo); // drop the label that is jumped

				// Now need to push a label number for the endif to use
				// to create the destination label for the jump past the
				// else code

				push_operation(IF_CONSTRUCTION_STACK_ITEM, labelNo + 1);

				// Allow statements after this one to indent
				previousStatementStartedBlock = true;
			}
			else
			{
#ifdef SCRIPT_DEBUG_INDENT_OUT
				Serial.print("...on its own");
#endif
				dropLabelStatement(pop_operation_count());
			}
			break;

		case WHILE_CONSTRUCTION_STACK_ITEM:

			labelNo = pop_operation_count();

			dropJumpCommand(labelNo);

			dropLabelStatement(labelNo + 1);
			break;

		case FOREVER_CONSTRUCTION_STACK_ITEM:

			labelNo = pop_operation_count();

			dropJumpCommand(labelNo);

			dropLabelStatement(labelNo + 1);
			break;

		default:
			result = ERROR_INDENT_OUTWARDS_HAS_INVALID_OPERATION_ON_STACK;
			break;
		}
	}
	// When we get here the indent of this statement should match the
	// the indent level pushed onto the operation stack when we started
	// this block
	if (indent != currentIndentLevel)
	{
		result = ERROR_INDENT_OUTWARDS_DOES_NOT_MATCH_ENCLOSING_STATEMENT_INDENT;
	}
	else
	{
		result = ERROR_OK;
	}

	return result;
}

//#define DEBUG_PYTHONISH

int pythonIshdecodeScriptLine(char *input)
{
#ifdef DEBUG_PYTHONISH
	Serial.printf("PythonIsh got line to decode: %s %d\n", input, strlen(input));
#endif

	if (*input == '#'){
		// lines starting with # are comments - ignore them
		return ERROR_OK;
	}
	
	if (*input == '{'){
		// lines starting with { are json commands - just send them through to the compiled output
		while (*input != 0)
		{
			HullOSProgramoutputFunction(*input);
			input++;
		}
		endCommand();
		return ERROR_OK;
	}


	// Set the shared buffer pointer to point to the statement being decoded
	bufferPos = input;

	int result;

	byte indent = skipInputSpaces();

	int commandNo = decodeCommandName(pythonishcommandNames);

	if (commandNo == COMMAND_EMPTY_LINE)
	{
		// Ignore empty lines
		return ERROR_OK;
	}

#ifdef SCRIPT_DEBUG

	Serial.print(previousStatementStartedBlock);
	Serial.print(" Current indent: ");
	Serial.print(currentIndentLevel);
	Serial.print("Indent: ");
	Serial.println(indent);

	Serial.print("Compiling: ");
	Serial.println(input);

#endif // SCRIPT_DEBUG

	// Find the position of the first item
	// sort out any outward indents

	if (storingProgram())
	{
		if (indent < currentIndentLevel)
		{
			// new statement is being outdented
			result = indentOutToNewIndentLevel(indent, commandNo);
			if (result == ERROR_OK)
			{
				result = processCommand(commandNo);
			}
		}
		else
		{
			if (indent > currentIndentLevel)
			{
				// Indenting the text
				// Only valid if we were pre-ceded by a
				// statement that can cause an indent
				if (previousStatementStartedBlock)
				{
					// It's OK to increase the indent if you're starting a new block
					// Set the new indent level for this block
					currentIndentLevel = indent;
					// Now process the command
					result = processCommand(commandNo);
				}
				else
				{
					// Inconsistent indents are bad
					result = ERROR_INDENT_INWARDS_WITHOUT_A_VALID_ENCLOSING_STATEMENT;
				}
			}
			else
			{
				// At the same level - just process the command
				result = processCommand(commandNo);
			}
		}
	}
	else
	{
		// Immediate mode
		result = processCommand(commandNo);
	}

	if (result != ERROR_OK)
	{
		abandonCompilation();

		if (storingProgram())
		{
			Serial.print("Line:  ");
			Serial.print(scriptLineNumber);
			Serial.print(" ");
		}

		Serial.print("Error: ");
		Serial.print(result);
		Serial.print(" ");
		Serial.println(input);
		printError(result);
	}

	endCommand();

	return result;
}

void testScript()
{
	beginCompilingStatements();
	clearVariables();

#ifdef SCRIPT_DEBUG

	Serial.print("Script test");

#endif // SCRIPT_DEBUG

	//  Serial.println(decodeCommandName("forward"));
	//  Serial.println(decodeCommandName("back"));
	//  Serial.println(decodeCommandName("wallaby"));

	//  decodeScriptLine("angry", dumpByte);
	//  decodeScriptLine("happy", dumpByte);

#ifdef SCRIPT_MOVE_TEST

	decodeScriptLine("move 50", dumpByte);
	decodeScriptLine("move 50 intime 10", dumpByte);
	decodeScriptLine("move", dumpByte);
	decodeScriptLine("move ", dumpByte);
	decodeScriptLine("move zz", dumpByte);
	decodeScriptLine("move 50zz", dumpByte);
	decodeScriptLine("move 50 intime", dumpByte);
	decodeScriptLine("move 50 intime 10", dumpByte);

#endif

	// #define SCRIPT_TURN_TEST

#ifdef SCRIPT_TURN_TEST

	decodeScriptLine("turn 50", dumpByte);
	decodeScriptLine("turn 50 intime 10", dumpByte);
	decodeScriptLine("turn", dumpByte);
	decodeScriptLine("turn ", dumpByte);
	decodeScriptLine("turn zz", dumpByte);
	decodeScriptLine("turn 50zz", dumpByte);
	decodeScriptLine("turn 50 intime", dumpByte);
	decodeScriptLine("turn 50 intime 10", dumpByte);

#endif

	// #define SCRIPT_ARC_TEST

#ifdef SCRIPT_ARC_TEST
	decodeScriptLine("arc 90, 180", dumpByte);
	decodeScriptLine("arc 90, 180 intime 100", dumpByte);
	decodeScriptLine("arc 90 , 80", dumpByte);
	decodeScriptLine("arc 90 ,80", dumpByte);
	decodeScriptLine("arc", dumpByte);
	decodeScriptLine("arc ", dumpByte);
	decodeScriptLine("arc zz", dumpByte);
	decodeScriptLine("arc 90", dumpByte);
	decodeScriptLine("arc 90,", dumpByte);
	decodeScriptLine("arc 90,zz", dumpByte);
	decodeScriptLine("arc 90+ 80", dumpByte);
#endif

	// #define SET_TEST
#ifdef SET_TEST
	decodeScriptLine("move x", dumpByte);
	decodeScriptLine("set x=99", dumpByte);
	decodeScriptLine("move x", dumpByte);
	decodeScriptLine("set x=x+1", dumpByte);
	decodeScriptLine("move x+10", dumpByte);

#endif

	// #define DELAY_TEST

#ifdef DELAY_TEST

	decodeScriptLine("delay 100", dumpByte);
	decodeScriptLine("delay", dumpByte);
	decodeScriptLine("delay ", dumpByte);
	decodeScriptLine("delay zz", dumpByte);
#endif

	// #define COLOUR_TEST

#ifdef COLOUR_TEST
	decodeScriptLine("colour 255,128,0", dumpByte);
	decodeScriptLine("colour 255,128,", dumpByte);
	decodeScriptLine("colour 255,128", dumpByte);
	decodeScriptLine("colour 255,", dumpByte);
	decodeScriptLine("colour 255", dumpByte);
	decodeScriptLine("colour ", dumpByte);
	decodeScriptLine("colour", dumpByte);

	decodeScriptLine("color 255,128,0", dumpByte);
	decodeScriptLine("color 255,128,", dumpByte);
	decodeScriptLine("color 255,128", dumpByte);
	decodeScriptLine("color 255,", dumpByte);
	decodeScriptLine("color 255", dumpByte);
	decodeScriptLine("color ", dumpByte);
	decodeScriptLine("color", dumpByte);

#endif

	// #define IF_TEST

#ifdef IF_TEST
	decodeScriptLine("if 1 > 20", dumpByte);
	decodeScriptLine("colour 255,128,0", dumpByte);
	decodeScriptLine("endif", dumpByte);
	decodeScriptLine("if 1 >= 20", dumpByte);
	decodeScriptLine("colour 255,128,255", dumpByte);
	decodeScriptLine("endif", dumpByte);

#endif

	// #define IF_ELSE_TEST

#ifdef IF_ELSE_TEST

	decodeScriptLine("do", dumpByte);
	decodeScriptLine("if %dist > 20", dumpByte);
	decodeScriptLine("    colour 255,0,0", dumpByte);
	decodeScriptLine("else", dumpByte);
	decodeScriptLine("    colour 0,255,0", dumpByte);
	decodeScriptLine("endif", dumpByte);
	decodeScriptLine("forever", dumpByte);

#endif

	// #define DO_TEST

#ifdef DO_TEST
	decodeScriptLine("set count = 0", dumpByte);
	decodeScriptLine("do", dumpByte);
	decodeScriptLine("colour 255,128,255", dumpByte);
	decodeScriptLine("delay 10", dumpByte);
	decodeScriptLine("colour 0,0,0", dumpByte);
	decodeScriptLine("delay 10", dumpByte);
	decodeScriptLine("set count = count + 1", dumpByte);
	decodeScriptLine("until count > 10", dumpByte);
#endif

	// #define WHILE_TEST

#ifdef WHILE_TEST
	decodeScriptLine("set count = 0", dumpByte);
	decodeScriptLine("while count < 10", dumpByte);
	decodeScriptLine("colour 255,128,255", dumpByte);
	decodeScriptLine("delay 10", dumpByte);
	decodeScriptLine("colour 0,0,0", dumpByte);
	decodeScriptLine("delay 10", dumpByte);
	decodeScriptLine("set count = count + 1", dumpByte);
	decodeScriptLine("endwhile", dumpByte);
#endif

	//  decodeScriptLine("turn 90", dumpByte);
	//  decodeScriptLine("arc 90, 180", dumpByte);
	//  decodeScriptLine("delay 10", dumpByte);
	//  decodeScriptLine("colour 255,0,255", dumpByte);
	//  decodeScriptLine("color 255,0,255", dumpByte);
	//  decodeScriptLine("pixel 255,255,255,0", dumpByte);
}
