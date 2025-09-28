#include <Arduino.h>
#include "HullOS.h"
#include "HullOSScript.h"
#include "RockStar.h"
#include "HullOSVariables.h"
#include "HullOSCommands.h"
#include "console.h"
#include "utils.h"
#include "messages.h"

#define MAX_TOKENS 10
#define MAX_TOKEN_LENGTH 20
#define MAX_DELIMITERS 5
#define MAX_DELIM_LENGTH 4

#define ROCKSTAR_DEBUG

int RockstarIshDecodeScriptLine(char *input);

void rockstarDecoderStart()
{
    displayMessage(F("Starting Rockstar decoder"));
}

struct LanguageHandler RockstarLanguage = {
    "Rockstar",
    rockstarDecoderStart,
    RockstarIshDecodeScriptLine,
    rockstarShowPrompt,
    '$'
};

void rockstarShowPrompt()
{
	displayMessage(F("R>"));
}


const char rockstarCommandNames[] =
    "angry,cross,mad#"                // ROCKSTAR_COMMAND_ANGRY       0
    "happy,pleased,mellow#"           // ROCKSTAR_COMMAND_HAPPY       1
    "move#"                           // ROCKSTAR_COMMAND_MOVE        2
    "turn#"                           // ROCKSTAR_COMMAND_TURN        3
    "arc#"                            // ROCKSTAR_COMMAND_ARC         4
    "delay#"                          // ROCKSTAR_COMMAND_DELAY       5
    "colour#"                         // ROCKSTAR_COMMAND_COLOUR      6
    "color#"                          // ROCKSTAR_COMMAND_COLOR       7
    "pixel#"                          // ROCKSTAR_COMMAND_PIXEL       8
    "set#"                            // ROCKSTAR_COMMAND_SET         9
    "if#"                             // ROCKSTAR_COMMAND_IF         10
    "do#"                             // ROCKSTAR_COMMAND_DO         11
    "while#"                          // ROCKSTAR_COMMAND_WHILE      12
    "intime#"                         // ROCKSTAR_COMMAND_INTIME     13
    "endif#"                          // ROCKSTAR_COMMAND_ENDIF      14
    "forever#"                        // ROCKSTAR_COMMAND_FOREVER    15
    "endwhile#"                       // ROCKSTAR_COMMAND_ENDWHILE   16
    "sound#"                          // ROCKSTAR_COMMAND_SOUND      17
    "until#"                          // ROCKSTAR_COMMAND_UNTIL      18
    "clear#"                          // ROCKSTAR_COMMAND_CLEAR      19
    "run#"                            // ROCKSTAR_COMMAND_RUN        20
    "background#"                     // ROCKSTAR_COMMAND_BACKGROUND 21
    "else#"                           // ROCKSTAR_COMMAND_ELSE       22
    "red#"                            // ROCKSTAR_COMMAND_RED        23
    "green#"                          // ROCKSTAR_COMMAND_GREEN      24
    "blue#"                           // ROCKSTAR_COMMAND_BLUE       25
    "yellow#"                         // ROCKSTAR_COMMAND_YELLOW     26
    "magenta#"                        // ROCKSTAR_COMMAND_MAGENTA    27
    "cyan#"                           // ROCKSTAR_COMMAND_CYAN       28
    "white#"                          // ROCKSTAR_COMMAND_WHITE      29
    "black#"                          // ROCKSTAR_COMMAND_BLACK      30
    "wait#"                           // ROCKSTAR_COMMAND_NO_WAIT       31
    "stop#"                           // ROCKSTAR_COMMAND_STOP       32
    "begin#"                          // ROCKSTAR_COMMAND_BEGIN      33
    "end#"                            // ROCKSTAR_COMMAND_END        34
    "write#"                          // ROCKSTAR_COMMAND_PRINT      35
    "print,scream,shout,whisper,say#" // ROCKSTAR_COMMAND_PRINTLN    36
    "break#"                          // ROCKSTAR_COMMAND_BREAK      37
    "duration#"                       // ROCKSTAR_COMMAND_DURATION   38
    "continue#"                       // ROCKSTAR_COMMAND_CONTINUE   39
    "angle#"                          // ROCKSTAR_COMMAND_ANGLE      40
    "save#"                           // ROCKSTAR_COMMAND_SAVE       41
    "load#"                           // ROCKSTAR_COMMAND_SAVE       42
    "dump#"                           // ROCKSTAR_COMMAND_DUMP       43
    "dance#"                          // ROCKSTAR_COMMAND_DANCE      44
    "an,a,the,my,your,our#"           // ROCKSTAR_COMMON_VARIABLE    45
    "is,are,am,was,were#"             // ROCKSTAR_IS_OPERATOR        46
    ;

int splitInput(const char *input, char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH])
{
    int tokenIndex = 0;
    int charIndex = 0;

    // Clear output tokens
    for (int i = 0; i < MAX_TOKENS; i++)
    {
        tokens[i][0] = '\0';
    }

    while (*input)
    {
        // Skip leading spaces
        while (*input == ' ')
            input++;

        // End of input
        if (*input == '\0')
            break;

        // Start filling token
        charIndex = 0;

        while (*input && *input != ' ')
        {
            if (charIndex >= MAX_TOKEN_LENGTH - 1)
            {
                displayMessage(F("Error: Token too long at token "));
                displayMessageWithNewline(F("%d"),tokenIndex);
                return tokenIndex;
            }
            tokens[tokenIndex][charIndex++] = *input++;
        }

        tokens[tokenIndex][charIndex] = '\0';
        tokenIndex++;

        if (tokenIndex >= MAX_TOKENS)
        {
            displayMessageWithNewline(F("Error: Too many tokens."));
            return tokenIndex;
        }
    }

    return tokenIndex;
}

inline bool atRockStarStatementEnd()
{
    char ch = *bufferPos;
    return (ch == 0) || (ch == '.') || (ch == '!') || (ch == STATEMENT_TERMINATOR);
}

char nextToken[MAX_TOKEN_LENGTH];

int copyIntoToken(int startPos)
{

    skipInputSpaces();

    int tokenOffset = startPos;

    while (true)
    {
        char ch = *bufferPos;

        if (atRockStarStatementEnd() || ch == ' ')
        {
            nextToken[tokenOffset] = 0;
#ifdef ROCKSTAR_DEBUG
            displayMessage(F("Got next token:%s\n"), nextToken);
#endif
            return ERROR_OK;
        }

        nextToken[tokenOffset] = ch;
        bufferPos++;
        tokenOffset++;
        if (tokenOffset>= MAX_TOKEN_LENGTH-1)
        {
            return ERROR_TOKEN_TOO_LARGE;
        }
    }
}

int getStartToken()
{
    return copyIntoToken(0);
}

int appendStringToToken(char * str){
    int tokenOffset = strlen(nextToken);

    while(true){

        char ch = *str;

        if(ch==0){
            nextToken[tokenOffset]=0;
            return ERROR_OK;
        }

        nextToken[tokenOffset]=ch;

        tokenOffset++;
        str++;

        if (tokenOffset>= MAX_TOKEN_LENGTH-1)
        {
            return ERROR_TOKEN_TOO_LARGE;
        }
    }
}

int appendToToken()
{
    return copyIntoToken(strlen(nextToken));
}

char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH];

int dropSimpleAssignment(char *variableName)
{

#ifdef ROCKSTAR_DEBUG
    displayMessage(F("Dropping simple assignment to %s\n"), nextToken);
#endif

    if (nextToken[0] == 0)
    {
        return ERROR_MISSING_VARIABLE_IN_SIMPLE_ASSIGNMENT;
    }

    // Simple assignment is followed directly by the value

    skipInputSpaces();

    if (checkIdentifier(nextToken) != VARIABLE_NAME_OK)
        return ERROR_INVALID_VARIABLE_NAME_IN_SET;

    int position;

    if (findVariable(bufferPos, &position) == VARIABLE_NOT_FOUND)
    {
        if (createVariable(bufferPos, &position) == NO_ROOM_FOR_VARIABLE)
        {
            return ERROR_TOO_MANY_VARIABLES;
        }
    }

    sendCommand("VS");

    sendCommand(nextToken);

    skipInputSpaces();

    HullOSProgramoutputFunction('='); // write the equals

    skipInputSpaces();

    int result = processValue();

    if (result == ERROR_OK)
    {
        endCommand();
    }

    return result;
}

int processRockstarCommand(int commandNo)
{
#ifdef ROCKSTAR_DEBUG

    displayMessage(F("Processing Rockstar Command:%d\n"), commandNo);

#endif

    switch (commandNo)
    {
    case ROCKSTAR_COMMAND_ANGRY: // angry
        return compileAngry();

    case ROCKSTAR_COMMAND_HAPPY: // happy
        return compileHappy();

    case ROCKSTAR_COMMAND_PRINT:
        return compilePrint();

    case ROCKSTAR_COMMAND_PRINTLN:
        return compilePrintln();

        /*
            case ROCKSTAR_COMMAND_MOVE: // move
                return compileMove();

            case ROCKSTAR_COMMAND_TURN: // turn
                return compileTurn();

            case ROCKSTAR_COMMAND_ARC: // arc
                return compileArc();

            case ROCKSTAR_COMMAND_DELAY: // delay
                return compileDelay();

            case ROCKSTAR_COMMAND_COLOUR: // colour
                return compileColour();

            case ROCKSTAR_COMMAND_COLOR: // color
                return compileColour();

            case ROCKSTAR_COMMAND_PIXEL: // pixel
                return compilePixel();

            case ROCKSTAR_COMMAND_IF: // if
                return compileIf();

            case ROCKSTAR_COMMAND_WHILE: // while
                return compileWhile();

            case ROCKSTAR_COMMAND_CLEAR: // clear
                return clearProgram();

            case ROCKSTAR_COMMAND_RUN: // run
                return runProgram();

            case ROCKSTAR_COMMAND_ELSE: // else
                return compileElse();

            case ROCKSTAR_COMMAND_FOREVER: // forever
                return compileForever();

            case ROCKSTAR_COMMAND_SET:
                return compileAssignment();

            case ROCKSTAR_COMMAND_RED:
            case ROCKSTAR_COMMAND_BLUE:
            case ROCKSTAR_COMMAND_GREEN:
            case ROCKSTAR_COMMAND_MAGENTA:
            case ROCKSTAR_COMMAND_CYAN:
            case ROCKSTAR_COMMAND_YELLOW:
            case ROCKSTAR_COMMAND_WHITE:
                return compileSimpleColor();

            case ROCKSTAR_COMMAND_BLACK:
                return compileBlack();

            case ROCKSTAR_COMMAND_NO_WAIT:
                return compileWait();

            case ROCKSTAR_COMMAND_STOP:
                return compileStop();

            case ROCKSTAR_COMMAND_BEGIN:
                return compileBegin();

            case ROCKSTAR_COMMAND_END:
                return compileEnd();

            case ROCKSTAR_COMMAND_SYSTEM_COMMAND:
                return compileDirectCommand();

            case ROCKSTAR_COMMAND_SOUND:
                return compileSound();

            case ROCKSTAR_COMMAND_BREAK:
                return compileBreak();

            case ROCKSTAR_COMMAND_CONTINUE:
                return compileContinue();

            case ROCKSTAR_COMMAND_SAVE:
                return compileProgramSave();

            case ROCKSTAR_COMMAND_LOAD:
                return compileProgramLoad();

            case ROCKSTAR_COMMAND_DUMP:
                return compileProgramDump();
            default:
                return compileAssignment();
        */
    }

    return ERROR_INVALID_COMMAND;
}

// Parses the poetic number at *bufferPos

int RockstarPoeticParse()
{

#ifdef ROCKSTAR_DEBUG
    displayMessage(F("Poetic parse from here %s\n"), bufferPos);
#endif

    int result = 0;

    skipInputSpaces();

    int digit = 0;

    while (true)
    {

        if (atRockStarStatementEnd())
        {
            result = (result * 10) + digit;
            return result;
        }

        char ch = *bufferPos;

        if (isAlpha(ch) || ch == '-')
        {
            digit++;
        }

        if (ch == ' ')
        {
            result = (result * 10) + digit;
            digit = 0;
            skipInputSpaces();
        }
        else
        {
            bufferPos++;
        }

#ifdef ROCKSTAR_DEBUG
        displayMessage(F("ch:%c digit:%d result:%d"), ch, digit, result);
#endif
    }
}

int RockstarIshDecodeScriptLine(char *input)
{
    char ch;

#ifdef ROCKSTAR_DEBUG
    displayMessage(F("Got a line of Rockstar to decode %s\n"), input);
#endif

    bufferPos = input;

    byte indent = skipInputSpaces();

    // Lines that start with a # are comments
    int result = getStartToken();

    if (result != ERROR_OK)
    {
#ifdef ROCKSTAR_DEBUG
        displayMessage(F("No token found\n"));
#endif
        return result;
    }

    if (nextToken[0] == 0)
    {
#ifdef ROCKSTAR_DEBUG
        displayMessage(F("Empty token found\n"));
#endif
        return ERROR_EMPTY_TOKEN;
    }

    if (endsWith(nextToken, "'s"))
    {
#ifdef ROCKSTAR_DEBUG
        displayMessage(F("Possessive assignment 2\n"));
#endif
        strip_end(nextToken, 2);
        resetScriptLine();
        return dropSimpleAssignment(nextToken);
    }

    if (endsWith(nextToken, "'re"))
    {
#ifdef ROCKSTAR_DEBUG
        displayMessage(F("Possessive assignment 3\n"));
#endif
        strip_end(nextToken, 3);
        resetScriptLine();
        return dropSimpleAssignment(nextToken);
    }

    // move back to the start of the line for input decoding

    bufferPos = input;

    int commandNo = decodeCommandName(rockstarCommandNames);

    if (commandNo == -1)
    {
#ifdef ROCKSTAR_DEBUG
        displayMessage(F("No command found. Checking for compound assignment\n"));
#endif

        // no command found - look for an assignment starting with a variable name
        // record the start of the name:

        char *varnamePos = bufferPos;

        bufferPos = bufferPos + strlen(nextToken);

        while(true){

            commandNo = decodeCommandName(rockstarCommandNames);

            if (commandNo != ROCKSTAR_IS_OPERATOR){
#ifdef ROCKSTAR_DEBUG
            displayMessage(F("Space terminated element in variable name. Moved input position to here:%s\n"), bufferPos);
#endif

                // Add a space to the end of the variable name 

                if(appendStringToToken(" ")!=ERROR_OK){
                    return ERROR_TOKEN_TOO_LARGE_FOR_BUFFER;
                }

                // add the next word from the variable name

                if(appendToToken()!=ERROR_OK){
                    return ERROR_TOKEN_TOO_LARGE_FOR_BUFFER;
                }
                continue;
            }
            else {
    #ifdef ROCKSTAR_DEBUG
                displayMessage(F("Is assignment of possibly poetic number\n"));
    #endif
                sendCommand("VS");

                sendCommand(nextToken);

                HullOSProgramoutputFunction('='); // write the equals

                skipInputSpaces();

                ch = *bufferPos;

                // if the value starts with a digit we parse it as an expression

                if (isdigit(ch))
                {

                    int result = processValue();

                    if (result == ERROR_OK)
                    {
                        endCommand();
                    }

                    return result;
                }

                int val = RockstarPoeticParse();

    #ifdef ROCKSTAR_DEBUG
                displayMessage(F("Poetic number to assign:%d\n"), val);
    #endif
                char numberBuffer[20];

                snprintf(numberBuffer, 20, "%d", val);

                sendCommand(numberBuffer);

                endCommand();
                return ERROR_OK;
            }
        }
    }

    result = processRockstarCommand(commandNo);

    if (result != ERROR_OK)
    {
        abandonCompilation();

        if (storingProgram())
        {
            displayMessage(F("Line:  %d "),scriptLineNumber);
        }

        displayMessage(F("Error: %d %s\n"), result);
        printError(result);
    }

    endCommand();

    return result;
}
