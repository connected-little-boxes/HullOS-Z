#include <Arduino.h>
#include <limits.h>
#include "utils.h"
#include "messages.h"
#include "HullOSCommands.h"

int rand_seed=1234;
int rand_mult=8121;
int rand_add=28411;
int rand_modulus=134456;

int localRand()
{
  rand_seed = (rand_mult * rand_seed + rand_add) % rand_modulus;
  return rand_seed;
}

int localRand(int limit)
{
    return localRand() % limit;
}

void localSrand(int seed)
{    
    rand_seed = seed;
}

int localRand(int low, int high)
{
    int diff = high - low;
    return localRand(diff) + low;
}

unsigned long ulongDiff(unsigned long end, unsigned long start)
{
    if (end >= start)
    {
        return end - start;
    }
    else
    {
        return ULONG_MAX - start + end + 1;
    }
}

bool strContains(char *searchMe, char *findMe)
{
    while (*searchMe != 0)
    {
        if (*searchMe == *findMe)
        {
            // found the start of the find string
            // try to match the rest of the find string from here
            char *s1 = searchMe;
            char *f1 = findMe;
            while (*s1 == *f1)
            {
                f1++;
                s1++;
                if (*f1 == 0)
                {
                    // hit the end of the find string - return with a win
                    return true;
                }
                if (*s1 == 0)
                {
                    // hit the end of the search string before we
                    // completed the find - return with a fail
                    return false;
                }
            }
        }
        searchMe++;
    }
    return false;
}

int getUnalignedInt(unsigned char *source)
{
    int result;
    memcpy((unsigned char *)&result, source, sizeof(int));
    return result;
}

float getUnalignedFloat(unsigned char *source)
{
    float result;
    memcpy((unsigned char *)&result, source, sizeof(float));
    return result;
}

void putUnalignedFloat(float fval, unsigned char *dest)
{
    unsigned char *source = (unsigned char *)&fval;
    memcpy(dest, source, sizeof(float));
}

float getUnalignedDouble(unsigned char *source)
{
    double result;
    memcpy((unsigned char *)&result, source, sizeof(double));
    return result;
}

void putUnalignedDouble(double dval, unsigned char *dest)
{
    unsigned char *source = (unsigned char *)&dval;
    memcpy(dest, source, sizeof(double));
}

int currentHeap;

void start_memory_monitor()
{
#if defined(WEMOSD1MINI) || defined(ESP32DOIT)
    currentHeap = ESP.getFreeHeap();
#else
    currentHeap = 0;
#endif
}

void display_memory_monitor( char * item)
{
#if defined(WEMOSD1MINI) || defined(ESP32DOIT)
    int newHeap = (int)ESP.getFreeHeap();
    int heapChange = currentHeap - newHeap;
    if(heapChange != 0) {
        displayMessage(F("   %s has grabbed %d of memory %d left\n"), item, heapChange, newHeap);
        currentHeap = newHeap;
    }
#else
    displayMessage(F("   Memory monitoring not enabled for this platform\n"), item);
#endif
}

void appendFormattedString(char * dest, int limit, const char *format, ...)
{
    // Use va_list to handle variable number of arguments
    va_list args;
    va_start(args, format);
    
    int availableSpace = limit - strlen(dest) - 1;

    if(availableSpace <= 0){
        return;
    }

    // Use vsnprintf to format the string
    char buffer[availableSpace];
    
    vsnprintf(buffer, availableSpace, format, args);

    strcat(dest,buffer);

    va_end(args);
}


void getProcID (char * dest, int length)
{
#if defined(ESP32DOIT)
    snprintf(dest, length, "%06lx", (unsigned long)ESP.getEfuseMac());
#endif

#if defined(WEMOSD1MINI)
    snprintf(dest, length, "%06lx", (unsigned long)ESP.getChipId());
#endif

#if defined(PICO)
	char id_buffer [(2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES) + 1];

	pico_get_unique_board_id_string(id_buffer,(2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES) + 1);

	snprintf(dest, length, "%s", id_buffer);
#endif
}

unsigned long getProcIDSalt()
{

#if defined(ESP32DOIT)
    return (unsigned long)ESP.getEfuseMac();
#endif

#if defined(WEMOSD1MINI)
   return (unsigned long)ESP.getChipId();
#endif

#if defined(PICO)
    int bufferLength = (2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES) + 1;
	char id_buffer [(2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES) + 1];
    unsigned long result = 0;

    for(int i=0;i<(2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES);i++)
    {
        result = result + id_buffer[i];
    }

    return result;
#endif

}

bool endsWith(const char *str, const char *suffix) {
    int lenStr = 0, lenSuffix = 0;

    // Manually calculate lengths (no strlen)
    while (str[lenStr]) lenStr++;
    while (suffix[lenSuffix]) lenSuffix++;

    // Suffix longer than string? Can't match
    if (lenSuffix > lenStr) return false;

    // Compare characters from the end
    for (int i = 0; i < lenSuffix; i++) {
        if (str[lenStr - lenSuffix + i] != suffix[i]) {
            return false;
        }
    }

    return true;
}

void strip_end(char *str, int n) {
    if (n < 0) return;  // Ignore negative n

    char *p = str;
    while (*p) p++;     // Move to the end of the string

    while (n-- > 0 && p > str) {
        --p;
        *p = '\0';
    }
}

File file;


File fileOpen(const char * path, char * mode){

#if defined(ARDUINO_ARCH_ESP32)

    char buff [REMOTE_FILENAME_BUFFER_SIZE+1];

    snprintf(buff,REMOTE_FILENAME_BUFFER_SIZE+1,"/%s",path);

    return LittleFS.open(buff,mode);

#else

    return LittleFS.open(path,mode);

#endif
}


void saveToFile(char * path, char * src){

	TRACELOG("Saving to a file:");
	TRACELOGLN(path);

    file = fileOpen(path, "w");
    file.printf("%s",src);
    file.close();
}

bool fileExists(char * path)
{
	TRACELOG("Checking file exists:");
	TRACELOGLN(path);

    file = fileOpen(path, "r");

    if(!file){
        return false;
    }

    file.close();
    return true;
}

bool removeFile(char *path){
    TRACELOG("remove a file:");
	TRACELOGLN(path);
    if (fileExists(path))
        {
        LittleFS.remove(path);
        if (fileExists(path)) 
            {
                displayMessage(F("Failed to delete:"));displayMessageWithNewline(path);
                return false;
            }
        displayMessage(F("Deleted:"));displayMessageWithNewline(path);
        return true;
        }
    displayMessage(path);displayMessageWithNewline(F(" does not exist"));
    return false;
}

bool loadFromFile(char * path, char * dest, int length){

	TRACELOG("Loading from a file:");
	TRACELOGLN(path);

	file = fileOpen(path, "r");

	if (!file || file.isDirectory())
	{
		TRACELOGLN("  failed to open the file");
		return false;
	}

    int pos = 0;

	while (file.available())
	{
        char ch = file.read();
        if(pos>=length){
            TRACELOGLN("  file larger than input buffer");
        	file.close();
            return false;
        }
        dest[pos]=(char)ch;
        pos++;
	}

    dest[pos]=PROGRAM_TERMINATOR;

	file.close();
    
	TRACELOGLN("Settings loaded successfully");

    return true;
}


void listLittleFSContents()
{
    displayMessageWithNewline(F("Listing LittleFS contents:"));
    if (!LittleFS.begin()) {
        displayMessageWithNewline(F("Failed to mount LittleFS"));
        return;
    }

    File root = fileOpen("/","r");
    if (!root || !root.isDirectory()) {
        displayMessageWithNewline(F("Failed to open root directory"));
        return;
    }

    File file = root.openNextFile();
    while (file) {
        displayMessage(F("File: %s = size %d\n"), file.name(), (int)file.size());
        file = root.openNextFile();
    }
}

void printFileContents(const char *filename) {
  // Attempt to open the file for reading
  File file = fileOpen(filename, "r");
  if (!file) {
    displayMessage(F("Failed to open file: "));
    displayMessageWithNewline(filename);
    return;
  }

  displayMessage(F("Contents of "));
  displayMessage(filename);
  displayMessageWithNewline(F(":"));

  // Read and print each character
  while (file.available()) {
    char ch = file.read();
    if(ch==STATEMENT_TERMINATOR){
        displayMessage(F("\n"));
    }
    else {
        Serial.write(ch);
    }
  }

  file.close();
  displayMessage(F("\n")); // Final newline after content
}
