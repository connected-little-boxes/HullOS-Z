#include <Arduino.h>
#include "HullOSScript.h"
#include "HullOSCommands.h"
#include "HullOSVariables.h"
#include "utils.h"
#include "messages.h"

const char* getErrorMessage(int code) {
    switch (code) {
        case ERROR_OK: return "No error.";
        case ERROR_MISSING_TIME_VALUE_IN_INTIME: return "Missing time value in 'intime'.";
        case ERROR_INVALID_INTIME: return "Invalid 'intime' value.";
        case ERROR_MISSING_MOVE_DISTANCE: return "Missing move distance.";
        case ERROR_NOT_IMPLEMENTED: return "Feature not implemented.";
        case ERROR_MISSING_ANGLE_IN_TURN: return "Missing angle in 'turn'.";
        case ERROR_INVAILD_ANGLE_SEPARATOR_IN_ARC: return "Invalid angle separator in 'arc'.";
        case VARIABLE_USED_BEFORE_IT_WAS_CREATED: return "Variable used before it was created.";
        case ERROR_INVALID_DIGIT_IN_NUMBER: return "Invalid digit in number.";
        case ERROR_INVALID_HARDWARE_READ_DEVICE: return "Invalid hardware read device.";
        case ERROR_INVALID_VARIABLE_NAME_IN_SET: return "Invalid variable name in 'set'.";
        case ERROR_TOO_MANY_VARIABLES: return "Too many variables declared.";
        case ERROR_NO_EQUALS_IN_SET: return "No equals sign in 'set' statement.";
        case ERROR_CHARACTERS_ON_THE_END_OF_THE_LINE_AFTER_WAIT: return "Unexpected characters after 'wait'.";
        case ERROR_INVALID_OPERATOR_IN_EXPRESSION: return "Invalid operator in expression.";
        case ERROR_MISSING_TIME_IN_DELAY: return "Missing time value in 'delay'.";
        case ERROR_MISSING_RED_VALUE_IN_COLOUR: return "Missing red value in 'colour'.";
        case ERROR_MISSING_GREEN_VALUE_IN_COLOUR: return "Missing green value in 'colour'.";
        case ERROR_MISSING_BLUE_VALUE_IN_COLOUR: return "Missing blue value in 'colour'.";
        case ERROR_MISSING_OPERATOR_IN_COMPARE: return "Missing operator in comparison.";
        case ERROR_ENDIF_BEFORE_IF: return "'endif' before 'if'.";
        case ERROR_INVALID_CONSTRUCTION_MATCHING_ENDIF: return "No matching 'if' for 'endif'.";
        case FOREVER_BEFORE_DO: return "'forever' before 'do'.";
        case ERROR_INVALID_CONSTRUCTION_MATCHING_FOREVER: return "Invalid block for 'forever'.";
        case UNTIL_BEFORE_DO: return "'until' before 'do'.";
        case ERROR_INVALID_CONSTRUCTION_MATCHING_UNTIL: return "Invalid block for 'until'.";
        case ERROR_INVALID_COMMAND: return "Invalid command.";
        case ERROR_MISSING_OPERATOR_IN_WHILE: return "Missing operator in 'while'.";
        case ERROR_INVALID_CONSTRUCTION_MATCHING_ENDWHILE: return "No matching 'while' for 'endwhile'.";
        case ENDWHILE_BEFORE_WHILE: return "'endwhile' before 'while'.";
        case ERROR_SCRIPT_INPUT_BUFFER_OVERFLOW: return "Script input buffer overflow.";
        case ERROR_INTIME_EXPECTED_BUT_NOT_SUPPLIED: return "'intime' expected but not supplied.";
        case ERROR_ELSE_WITHOUT_IF: return "'else' without matching 'if'.";
        case ERROR_INDENT_OUTWARDS_WITHOUT_ENCLOSING_COMMAND: return "Indent outwards without enclosing command.";
        case ERROR_INDENT_INWARDS_WITHOUT_A_VALID_ENCLOSING_STATEMENT: return "Indent inwards without valid enclosing statement.";
        case ERROR_INDENT_OUTWARDS_HAS_INVALID_OPERATION_ON_STACK: return "Indent outwards with invalid operation on stack.";
        case ERROR_INDENT_OUTWARDS_DOES_NOT_MATCH_ENCLOSING_STATEMENT_INDENT: return "Indent outwards mismatch.";
        case ERROR_MISSING_CLOSE_QUOTE_ON_PRINT: return "Missing close quote on 'print'.";
        case ERROR_MISSING_PITCH_VALUE_IN_SOUND: return "Missing pitch value in 'sound'.";
        case ERROR_MISSING_DURATION_VALUE_IN_SOUND: return "Missing duration in 'sound'.";
        case ERROR_NOT_A_WAIT_AT_THE_END_OF_SOUND: return "Missing 'wait' at end of 'sound'.";
        case ERROR_BEGIN_WHEN_COMPILING_PROGRAM: return "'begin' not allowed inside another program.";
        case ERROR_END_WHEN_NOT_COMPILING_PROGRAM: return "'end' outside of program.";
        case ERROR_STOP_WHEN_COMPILING_PROGRAM: return "'stop' not allowed inside program definition.";
        case ERROR_RUN_WHEN_COMPILING_PROGRAM: return "'run' not allowed inside program definition.";
        case ERROR_CLEAR_WHEN_COMPILING_PROGRAM: return "'clear' not allowed inside program definition.";
        case ERROR_FOREVER_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'forever' used outside a program.";
        case ERROR_WHILE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'while' used outside a program.";
        case ERROR_IF_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'if' used outside a program.";
        case ERROR_ELSE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'else' used outside a program.";
        case ERROR_ELSE_MUST_BE_PART_OF_AN_INDENTED_BLOCK: return "'else' must be indented.";
        case ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_BREAK: return "'break' without enclosing loop.";
        case ERROR_BREAK_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'break' outside a program.";
        case ERROR_INVALID_COMMAND_AFTER_DURATION_SHOULD_BE_NO_WAIT: return "Expected 'nowait' after 'duration'.";
        case ERROR_SECOND_COMMAND_IN_SOUND_IS_NOT_DURATION: return "Second command in 'sound' is not 'duration'.";
        case ERROR_CONTINUE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM: return "'continue' outside a program.";
        case ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_CONTINUE: return "'continue' without enclosing loop.";
        case ERROR_NO_RADIUS_IN_ARC: return "Missing radius in 'arc'.";
        case ERROR_NO_ANGLE_IN_ARC: return "Missing angle in 'arc'.";
        case ERROR_NO_COMMAND_START_CHAR: return "Missing command start character.";
        case ERROR_INVALID_VARIABLE_NAME_IN_SET_COMMON_VARIABLE: return "Invalid variable name in common variable 'set'.";
		case ERROR_NO_ASSIGNMENT_NAME_IN_SET: return "No valid assignment name in 'set'.";
		case ERROR_TOKEN_TOO_LARGE: return "Token too large.";
		case ERROR_EMPTY_TOKEN: return "Missing token.";
		case ERROR_MISSING_VARIABLE_IN_SIMPLE_ASSIGNMENT: return "Missing variable in simple assignment.";
		case ERROR_MISSING_QUOTE_IN_FILENAME_STRING_START: return "Missing quote in filename start";
		case ERROR_MISSING_QUOTE_IN_FILENAME_STRING_END: return "Missing quote in filename end";
		case ERROR_FILENAME_TOO_LONG: return "Filename too long";
		case ERROR_SAVE_NOT_AVAILABLE_WHEN_COMPILING: return "Save not available when compiling. End compile before saving";
		case ERROR_LOAD_NOT_AVAILABLE_WHEN_COMPILING: return "Load not available when compiling. End compile before loading";
		case ERROR_FILE_LOAD_FAILED: return "File load failed";
		case ERROR_DUMP_NOT_AVAILABLE_WHEN_COMPILING: return "Dump not available when compiling. End compile before dumping";
		case ERROR_FILE_DUMP_FAILED: return "File dump failed";
		case ERROR_TOKEN_TOO_LARGE_FOR_BUFFER: return "Token too large for buffer";
		case ERROR_MISSING_CLOSE_QUOTE_ON_SEND: return "Missing close quote on send";
		case ERROR_NO_WAIT_SHOULD_BE_THE_LAST_THING_ON_A_LINE: return "Nowait should be the last thing on the line";
        default: return "Unknown error.";
    }
}

void printError(int code) {
    displayMessage("Error ");
    displayMessage("%d",code);
    displayMessage(": ");
    displayMessageWithNewline(getErrorMessage(code));
}

int scriptInputBufferPos;

// The line number in the script
// Used when reporting errors

int scriptLineNumber;

// Flag to indicate an error has been detected
// Used for error reporting

bool programError;

// The start position of the command in the input buffer
// Set by decodeCommand
// Shared with all the functions below
char *commandStartPos;

// THIS MEANS THAT THIS COMPILER IS NOT REENTRANT

// The position in the input buffer
// Set to the start of the command buffer by decodeScriptLine
// Shared with all the functions below and updated by them

// THIS MEANS THAT THIS COMPILER IS NOT REENTRANT

char *bufferPos;

// The position in the command names
// Set to the start of the command names by decodeScriptLine
// Shared with all the functions below and updated by some

// THIS ALSO MEANS THAT THIS COMPILER IS NOT REENTRANT

int scriptCommandPos;

// The indent level of the current statement
// Starts at 0 and increases with each block construction
// Shared with all the functions below and updated by some

// THIS ALSO MEANS THAT THIS COMPILER IS NOT REENTRANT

int currentIndentLevel;

// True if the previous statement started a block
// This statement is allowed to set a new indent level
//

bool displayErrors = true;

bool previousStatementStartedBlock = false;

void resetScriptLine()
{
	scriptInputBufferPos = 0;
}

int skipInputSpaces()
{
	int result = 0;

	while (*bufferPos == ' ')
	{
		result++;
		bufferPos++;
	}
	return result;
}

bool spinToCommandEnd(const char * commandNames)
{
	while (true)
	{
		char ch = *(commandNames + scriptCommandPos);

		if (ch == 0)
			// end of the string in memory
			return false;

		if (ch == COMMAND_NAME_TERMINATOR)
		{
			// move past the terminator
			scriptCommandPos++;
			return true;
		}

		// move to the next character
		scriptCommandPos++;
	}
}

bool spinToNextCommandAlias(const char * commandNames)
{
	while (true)
	{
		char ch = commandNames[scriptCommandPos];

		if (ch == 0)
			// end of the string in memory
			return false;

		if (ch == COMMAND_ALIAS_SEPARATOR)
		{
			// move past the separator
			scriptCommandPos++;
			return true;
		}

		if (ch == COMMAND_NAME_TERMINATOR)
		{
			// move past the terminator
			return false;
		}

		// move to the next character
		scriptCommandPos++;
	}
}


// #define COMPARE_COMMAND_DEBUG

ScriptCompareCommandResult compareCommand(const char * commandNames)
{
	// Start at the buffer position

	char *comparePos = bufferPos;
	char *startPos = bufferPos;

	while (true)
	{
		char ch = commandNames[scriptCommandPos];

#ifdef COMPARE_COMMAND_DEBUG
		displayMessage(ch);
#endif

		if (ch == 0)
			return END_OF_COMMANDS;

 		char inputCh = toLowerCase(*comparePos);

		// If we have reached the end of the command and the end of the input at the same time
		// we have a match. End of the input is a space or the end of the line

		if (ch == COMMAND_ALIAS_SEPARATOR | ch == COMMAND_NAME_TERMINATOR){
			// found the end of the command name
			// have we also reached the end of the input string
			if (inputCh== 0 || inputCh==' ' ){
				// we've found an alias for the command
				bufferPos = comparePos;
	#ifdef COMPARE_COMMAND_DEBUG
				displayMessageWithNewline("..match with command");
	#endif
				return COMMAND_MATCHED;
			}			
		}

		if (ch != inputCh)
		{
			// this doesn't match the command - but it might match an alias
			// see if we can find one
			// move past the alias terminator

			if (spinToNextCommandAlias(commandNames))
			{
				// reset the input position to the start
				comparePos = bufferPos;
#ifdef COMPARE_COMMAND_DEBUG
				displayMessageWithNewline("..separator");
#endif
			}
			else
			{
#ifdef COMPARE_COMMAND_DEBUG
				displayMessageWithNewline("..fail");
#endif
				return COMMAND_NOT_MATCHED;
			}
		}
		else {
			scriptCommandPos++;
			comparePos++;
		}
	}
}

// Decodes the command held in the area of memory referred to by bufferPos
int decodeCommandName(const char * commandNames)
{
 	// Set the position in the command list to the start of the list
	scriptCommandPos = 0;

	// Set the command counter to 0
	int commandNumber = 0;

	skipInputSpaces();

	// ignore empty lines
	if (*bufferPos == 0)
		return COMMAND_EMPTY_LINE;

	// Set commandStartPos to point to the start of the statement being decoded
	// Used when decoding colour names

	commandStartPos = bufferPos;

	// it is a system command - just return this immediately

	if (*bufferPos == '*')
	{
		// skip past the *
		bufferPos++;
		// return the command type
		return COMMAND_SYSTEM_COMMAND;
	}

	while (true)
	{
		char *nameStart = bufferPos;

		ScriptCompareCommandResult result = compareCommand(commandNames);

		switch (result)
		{
		case COMMAND_MATCHED:
//			displayMessage("Command matched: %d\n", commandNumber);
			return commandNumber;

		case COMMAND_NOT_MATCHED:
			if (!spinToCommandEnd(commandNames))
			{
#ifdef SCRIPT_DEBUG
				displayMessage("Command not matched: %d\n", commandNumber);
#endif
				return -1;
			}
			commandNumber++;
			break;

		case END_OF_COMMANDS:
#ifdef SCRIPT_DEBUG
			displayMessage("Hit the end\n");
#endif
			return -1;
			break;
		}
	}
}

void writeBytesFromBuffer(int length)
{
	for (int i = 0; i < length; i++)
	{
		HullOSProgramoutputFunction(*bufferPos);
		bufferPos++;
	}
}

void writeMatchingStringFromBuffer(char *string)
{
	while (*string)
	{
		HullOSProgramoutputFunction(*bufferPos);
		bufferPos++;
		string++;
	}
}

#ifdef SCRIPT_DEBUG

#define DUMP_BUFFER_SIZE 20
#define DUMP_BUFFER_LIMIT DUMP_BUFFER_SIZE - 1

char dumpBuffer[DUMP_BUFFER_SIZE];

int dumpBufferPos = 0;

boolean addDumpByte(byte b)
{
	if (dumpBufferPos < DUMP_BUFFER_LIMIT)
	{
		dumpBuffer[dumpBufferPos] = b;
		dumpBufferPos++;
		return true;
	}
	return false;
}

void displayDump()
{
	dumpBuffer[dumpBufferPos] = 0;
	displayMessageWithNewline(dumpBuffer);
	dumpBufferPos = 0;
}

void dumpByte(byte b)
{
	storeProgramByte(b);
	if (b == STATEMENT_TERMINATOR)
	{
		displayDump();
	}
	else
	{
		addDumpByte((char)b);
	}
}

#endif

int processSingleValue()
{
	skipInputSpaces();

	if (isVariableNameStart(bufferPos))
	{
		// its a variable
		int position;

		if (findVariable(bufferPos, &position) == VARIABLE_NOT_FOUND)
		{
			return VARIABLE_USED_BEFORE_IT_WAS_CREATED;
		}

		// copy the variable into the instruction

		int variableLength = getVariableNameLength(position);

		for (int i = 0; i < variableLength; i++)
		{
			HullOSProgramoutputFunction(*bufferPos);
			bufferPos++;
		}
		return ERROR_OK;
	}

	if (isdigit(*bufferPos) | (*bufferPos == '+') | (*bufferPos == '-'))
	{
		bool firstch = true;

		while (true)
		{
			char ch = *bufferPos;

			if (ch < '0' | ch > '9')
			{
				if (firstch)
				{
					if (ch != '+' && ch != '-')
					{
						return ERROR_INVALID_DIGIT_IN_NUMBER;
					}
				}
				else
				{
					return ERROR_OK;
				}
			}
			HullOSProgramoutputFunction(ch);
			firstch = false;
			bufferPos++;
		}
	}

	if (*bufferPos == READING_START_CHAR)
	{
		// Move past the start character

		bufferPos++;

		struct reading *reader = getReading(bufferPos);

		if (reader == NULL)
		{
			return ERROR_INVALID_HARDWARE_READ_DEVICE;
		}

		// Drop out the char to start the hardware name

		HullOSProgramoutputFunction(READING_START_CHAR);

		// copy the variable into the instruction

		int readerLength = strlen(reader->name);

		for (int i = 0; i < readerLength; i++)
		{
			HullOSProgramoutputFunction(*bufferPos);
			bufferPos++;
		}
		return ERROR_OK;
	}
	return ERROR_NO_COMMAND_START_CHAR;
}

int processValue()
{
	int result = processSingleValue();

	if (result != ERROR_OK)
		return result;

	skipInputSpaces();

	if (*bufferPos == 0)
		// Just a single value - no expression
		return ERROR_OK;

	if (validOperator(*bufferPos))
	{
		// write out the operator
		HullOSProgramoutputFunction(*bufferPos);

		// move past the operator
		bufferPos++;

		skipInputSpaces();

		// process the second value
		return processSingleValue();
	}

	previousStatementStartedBlock = false;

	return ERROR_OK;
}

void sendCommand(const char *command)
{
	int pos = 0;

	while (true)
	{
		byte b = command[pos];

		if (b == 0)
			break;

		HullOSProgramoutputFunction(b);
		pos++;
	}
}

void endCommand()
{
	HullOSProgramoutputFunction(STATEMENT_TERMINATOR);
}

void abandonCompilation()
{
	programError = true;
}

const char angryCommand[] = "PF20";

int compileAngry()
{
#ifdef SCRIPT_DEBUG
	displayMessageWithNewline(F("Compiling angry: "));
#endif // SCRIPT_DEBUG

	sendCommand(angryCommand);
	previousStatementStartedBlock = false;
	return ERROR_OK;
}

const char happyCommand[] = "PF1";

int compileHappy()
{
#ifdef SCRIPT_DEBUG
	displayMessageWithNewline(F("Compiling happy: "));
#endif // SCRIPT_DEBUG

	sendCommand(happyCommand);
	previousStatementStartedBlock = false;
	return ERROR_OK;
}

// compile a print statement
// The command is followed by an expression or a string of text enclosed in " characters
//
int compilePrint()
{
	// Not allowed to indent after a print
	previousStatementStartedBlock = false;

	// first character of the write command
	HullOSProgramoutputFunction('W');

	skipInputSpaces();

	if (*bufferPos == '"')
	{
		// start of a message - just drop out the string of text
		HullOSProgramoutputFunction('T');

		bufferPos++; // skip the starting double quote
		while (*bufferPos != 0 && *bufferPos != '"')
		{
			HullOSProgramoutputFunction(*bufferPos);
			bufferPos++;
		}
		if (*bufferPos == 0)
		{
			return ERROR_MISSING_CLOSE_QUOTE_ON_PRINT;
		}
		else
		{
			return ERROR_OK;
		}
	}
	else
	{
		// start of a value - just drop out the expression
		HullOSProgramoutputFunction('V');
		// dropping a value - just process it
		return processValue();
	}
}

int compileSend()
{
	// Not allowed to indent after a send
	previousStatementStartedBlock = false;

	// first character of the send command
	// The command is in the "remote" family

	HullOSProgramoutputFunction('R');

	skipInputSpaces();

	if (*bufferPos == '"')
	{
		// start of a message - just drop out the string of text
		HullOSProgramoutputFunction('T');

		bufferPos++; // skip the starting double quote
		while (*bufferPos != 0 && *bufferPos != '"')
		{
			HullOSProgramoutputFunction(*bufferPos);
			bufferPos++;
		}
		if (*bufferPos == 0)
		{
			return ERROR_MISSING_CLOSE_QUOTE_ON_SEND;
		}
		else
		{
			return ERROR_OK;
		}
	}
	else
	{
		// start of a value - just drop out the expression
		HullOSProgramoutputFunction('V');
		// dropping a value - just process it
		return processValue();
	}
}

const char newlineCommand[] = "WL";

int compilePrintln()
{
	// Not allowed to indent after a println
	previousStatementStartedBlock = false;

	compilePrint();

	// Going to follow this command with another
	endCommand();

	sendCommand(newlineCommand);
	return ERROR_OK;
}

const char clearCommand[] = "RC";
const char beginCommand[] = "RM";

struct stackItem
{
	byte constructionType;
	int count;
	byte indentLevel;
};

#define STACK_SIZE 10

struct stackItem operation[STACK_SIZE];

int operationStackPointer;

int labelCounter;

void beginCompilingStatements()
{
	currentIndentLevel = 0;
	previousStatementStartedBlock = false;
	operationStackPointer = 0;
	labelCounter = 0;
	resetScriptLine();
	scriptLineNumber = 1; // start at the first line
	programError = false; // indicate that no errors were detected
}

void startCompiling()
{
	beginCompilingStatements();
	sendCommand(clearCommand);
	endCommand();
	sendCommand(beginCommand);
}

void dropValue(int value)
{
	while (true)
	{
		char ch = '0' + (value % 10);
		HullOSProgramoutputFunction(ch);
		value = value / 10;
		if (value == 0)
			break;
	}
	return;
}

// Push an operation onto the operation stack.
// This manages the if, do and while constructions
//
void push_operation(byte type, byte count)
{
	operation[operationStackPointer].constructionType = type;
	operation[operationStackPointer].count = count;
	operation[operationStackPointer].indentLevel = currentIndentLevel;
	operationStackPointer++;
}

// Get the type of the top operation without removing anything from the stack
// We need to use this to check to make sure that the end element of a construction
// matches the start element.

bool operation_stack_empty()
{
	return operationStackPointer == 0;
}

byte top_operation_type()
{
	if (operationStackPointer == 0)
		return EMPTY_STACK;

	return operation[operationStackPointer - 1].constructionType;
}

int top_operation_label()
{
	if (operationStackPointer == 0)
		return EMPTY_STACK;

	return operation[operationStackPointer - 1].count;
}

byte top_operation_indent_level()
{
	if (operationStackPointer == 0)
		return EMPTY_STACK;

	return operation[operationStackPointer - 1].indentLevel;
}

// Get the top value on the operation stack
int pop_operation_count()
{
	operationStackPointer--;
	return operation[operationStackPointer].count;
}

const char labelCommand[] = "CLl";

void dropLabel(int labelCounter)
{
	// first character of the label
	sendCommand(labelCommand);
	dropValue(labelCounter);
}

void dropLabelStatement(int labelCounter)
{
	dropLabel(labelCounter);
	endCommand();
}

void pushLabel(byte labelType)
{
	labelCounter++; // move on to the next construction
	push_operation(labelType, labelCounter);
	dropLabel(labelCounter);
}

const char jumpCommand[] = "CJl";

void dropJump(int labelCounter)
{
	sendCommand(jumpCommand);
	dropValue(labelCounter);
}

void dropJumpCommand(int labelCounter)
{
	dropJump(labelCounter);
	endCommand();
}

const char endCommandText[] = "RX";

const char failedCommandText[] = "RA";

void endCompilingStatements()
{
	if (programError)
	{
		sendCommand(failedCommandText);
		displayMessageWithNewline("Errors");
	}
	else
	{
		sendCommand(endCommandText);
		displayMessageWithNewline("OK");
	}
}

// Drops a comparison statement
int dropComparisonStatement(int labelNo, bool trueTest)
{
	HullOSProgramoutputFunction('C');

	if (trueTest)
		HullOSProgramoutputFunction('T');
	else
		HullOSProgramoutputFunction('F');

	skipInputSpaces();

	// Get the first value in the logical expression
	int result = processSingleValue();

	if (result != ERROR_OK)
		return result;

	// Skip to the logical operator
	skipInputSpaces();

	// Get the logical operator
	struct logicalOp *ifOp = findLogicalOp(bufferPos);

	// Abandon if there is no matching logical operator
	if (ifOp == NULL)
	{
		// no logical operator
		return ERROR_MISSING_OPERATOR_IN_COMPARE;
	}

	// Write out the logical operator
	writeMatchingStringFromBuffer(ifOp->operatorCh);

	// Skip to the second operand
	skipInputSpaces();

	// process the second operand
	result = processSingleValue();

	if (result != ERROR_OK)
		return result;

	// if we get here the condition is valid and we need to drop out the destination label
	// for the branch past the

	HullOSProgramoutputFunction(','); // write the comma

	// Drop out the first character of the label (which is l)
	HullOSProgramoutputFunction('l');
	// drop the label counter value
	dropValue(labelNo);

	return ERROR_OK;
}

int compileIf()
{

	if (!storingProgram())
	{
		return ERROR_IF_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

#ifdef SCRIPT_DEBUG
	displayMessage(F("Compiling if: "));
#endif // SCRIPT_DEBUG

	labelCounter++; // move on to the next label

	// Add the start of the if to the operation stack

	push_operation(IF_CONSTRUCTION_STACK_ITEM, labelCounter);

	int result = dropComparisonStatement(labelCounter, false);

	labelCounter++; // reserve a label for use by else - if any

	previousStatementStartedBlock = true;

	return result;
}

int compileElse()
{
#ifdef SCRIPT_DEBUG
	displayMessage(F("Compiling else: "));
#endif // SCRIPT_DEBUG
	if (!storingProgram())
	{
		return ERROR_ELSE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	return ERROR_OK;
}

int compileWhile()
{
#ifdef SCRIPT_DEBUG
	displayMessage(F("Compiling while: "));
#endif // SCRIPT_DEBUG

	if (!storingProgram())
	{
		return ERROR_WHILE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	// First drop out a label so that
	// we can branch back to the top

	pushLabel(WHILE_CONSTRUCTION_STACK_ITEM);

	// Going to follow this command with another
	endCommand();

	labelCounter++; // move on to the next label

	// Now insert the branch past the loop

	previousStatementStartedBlock = true;

	return dropComparisonStatement(labelCounter, false);
}

int compileForever()
{

#ifdef SCRIPT_DEBUG
	displayMessage(F("Compiling forever: "));
#endif // SCRIPT_DEBUG

	if (!storingProgram())
	{
		return ERROR_FOREVER_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	// First drop out a label so that
	// we can branch back to the top

	pushLabel(FOREVER_CONSTRUCTION_STACK_ITEM);

	labelCounter++; // move on to the next label

	// Now insert the branch past the loop

	previousStatementStartedBlock = true;

	return ERROR_OK;
}

// finds the most recent loop construction on the
// operation stack. Returns -1 if no suitable construction found
//

#define NO_LABEL_FOR_LOOP_ON_STACK -1

int findTopLoopConstructionLabel()
{
	// Start the search at the top of the stack
	// Remember that
	int searchStackPointer = operationStackPointer;

	// If the operation stack pointer is zero there is nothing
	// on the stack
	while (searchStackPointer > 0)
	{
		searchStackPointer--; // climb down the stack
							  // pointer aways points to next free location
		byte constructionType = operation[searchStackPointer].constructionType;

		if ((constructionType == WHILE_CONSTRUCTION_STACK_ITEM) || (constructionType == FOREVER_CONSTRUCTION_STACK_ITEM))
		{
			// found a loop construction
			// return the label from that loop
			return operation[searchStackPointer].count;
		}
	}

	// If we get here there is no loop construction available - which is an error

	return NO_LABEL_FOR_LOOP_ON_STACK;
}

int compileBreak()
{

	// Not allowed to indent after a break
	previousStatementStartedBlock = false;

#ifdef SCRIPT_DEBUG
	displayMessage(F("Compiling break: "));
#endif // SCRIPT_DEBUG

	if (!storingProgram())
	{
		return ERROR_BREAK_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	int operation_label = findTopLoopConstructionLabel();

	if (operation_label == NO_LABEL_FOR_LOOP_ON_STACK)
		return ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_BREAK;

	// first label value is the jump for the loop repeat
	// next label value is the label after the end of the loop

	dropJump(operation_label + 1);
	return ERROR_OK;
}

int compileContinue()
{

#ifdef SCRIPT_DEBUG
	displayMessage(F("Compiling break: "));
#endif // SCRIPT_DEBUG

	// Not allowed to indent after a continue
	previousStatementStartedBlock = false;

	if (!storingProgram())
	{
		return ERROR_CONTINUE_CANNOT_BE_USED_OUTSIDE_A_PROGRAM;
	}

	int operation_label = findTopLoopConstructionLabel();

	if (operation_label == NO_LABEL_FOR_LOOP_ON_STACK)
		return ERROR_NO_LABEL_FOR_LOOP_ON_STACK_IN_CONTINUE;

	// first label value is the jump for the loop repeat

	dropJump(operation_label);

	return ERROR_OK;
}

/// Program control commands - not part of the script
//

const char clearVariablesCommand[] = "VC";

int clearProgram()
{
#ifdef SCRIPT_DEBUG
	displayMessage(F("Performing clear program: "));
#endif // SCRIPT_DEBUG

	if (storingProgram())
	{
		return ERROR_CLEAR_WHEN_COMPILING_PROGRAM;
	}

	sendCommand(clearVariablesCommand);

	return ERROR_OK;
}

const char runCommand[] = "RS";

int runProgram()
{
#ifdef SCRIPT_DEBUG
	displayMessage(F("Performing run program: "));
#endif // SCRIPT_DEBUG

	if (storingProgram())
	{
		return ERROR_RUN_WHEN_COMPILING_PROGRAM;
	}

	sendCommand(runCommand);

	return ERROR_OK;
}

const char waitCommand[] = "CA";

int compileWait()
{
	// Not allowed to indent after a wait
	previousStatementStartedBlock = false;

	

	sendCommand(waitCommand);

	return ERROR_OK;
}

const char stopCommand[] = "RH";

int compileStop()
{

	// Not allowed to indent after a sound
	previousStatementStartedBlock = false;

	if (storingProgram())
	{
		return ERROR_STOP_WHEN_COMPILING_PROGRAM;
	}

	sendCommand(stopCommand);
	return ERROR_OK;
}


int compileBegin()
{
	// Not allowed to indent after a begin
	previousStatementStartedBlock = false;

	startCompiling();

	return ERROR_OK;
}

int getProgramFilenameFromCode()
{

	skipInputSpaces();

	char ch = *bufferPos;

	if (ch != '"')
	{
		return ERROR_MISSING_QUOTE_IN_FILENAME_STRING_START;
	}

	// move past the quote
	bufferPos++;

	// Now copy the filename into the buffer

	int pos = 0;

	while (true)
	{

		ch = *bufferPos;

//		displayMessage("  Copying:%d %c\n", ch, ch);

		if (ch == 0)
		{
			return ERROR_MISSING_QUOTE_IN_FILENAME_STRING_END;
		}

		if (ch == '"')
		{
			// terminate the output filename
			HullOScommandsFilenameBuffer[pos] = 0;
			break;
		}

		if (pos >= REMOTE_FILENAME_BUFFER_SIZE - 1)
		{
			return ERROR_FILENAME_TOO_LONG;
		}

		HullOScommandsFilenameBuffer[pos] = ch;
		bufferPos++;
		pos++;
	}

	return ERROR_OK;
}

int compileEnd()
{
	// Not allowed to indent after a end
	previousStatementStartedBlock = false;

	if (!storingProgram())
	{
		return ERROR_END_WHEN_NOT_COMPILING_PROGRAM;
	}

	int result = getProgramFilenameFromCode();

	if (result != ERROR_OK)
	{
		clearHullOSFilename();
	}

	endCompilingStatements();

	return ERROR_OK;
}

int compileDirectCommand()
{
	// Not allowed to indent after a sound
	previousStatementStartedBlock = false;

	while (*bufferPos)
	{
		HullOSProgramoutputFunction(*bufferPos);
		bufferPos++;
	}
	return ERROR_OK;
}

int compileProgramSave()
{

	displayMessageWithNewline("Compiling program save command");

	if (storingProgram())
	{
		return ERROR_SAVE_NOT_AVAILABLE_WHEN_COMPILING;
	}

	int result = getProgramFilenameFromCode();

	if (result != ERROR_OK)
	{
		return result;
	}

	// If we get here the filename is valid

	displayMessage("Storing the program in:%s\n", HullOScommandsFilenameBuffer);
	saveToFile(HullOScommandsFilenameBuffer, HullOScodeCompileOutput);

	return ERROR_OK;
}

int compileProgramLoad(bool clearVariablesBeforeRun)
{
	displayMessageWithNewline("Compiling program load or chain command");

	int result = getProgramFilenameFromCode();

	if (result != ERROR_OK)
	{
		return result;
	}

	if (!fileExists(HullOScommandsFilenameBuffer))
	{
		return ERROR_FILE_LOAD_FAILED;
	}
	
	if(clearVariablesBeforeRun){
		sendCommand("RF");
	}
	else 
	{
		sendCommand("RE");
	}

	sendCommand(HullOScommandsFilenameBuffer);

	return ERROR_OK;
}

int compileProgramDump()
{
	displayMessageWithNewline("Compiling program dump command");

	if (storingProgram())
	{
		return ERROR_DUMP_NOT_AVAILABLE_WHEN_COMPILING;
	}

	int result = getProgramFilenameFromCode();

	if (result != ERROR_OK)
	{
		return result;
	}

	if (!fileExists(HullOScommandsFilenameBuffer))
	{
		return ERROR_FILE_DUMP_FAILED;
	}

	sendCommand("RD");
	sendCommand(HullOScommandsFilenameBuffer);

	return ERROR_OK;
}
