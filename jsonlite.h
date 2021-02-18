#include <Arduino.h>


enum EJsonParseSteps
{
    EJsonParseStep_None = 0,
    EJsonParseStep_ParamName,
    EJsonParseStep_ParamValue,
};

struct SJsonSkipInfo
{
    uint16_t objectsDepth : 8;
    uint16_t arrayDepth : 7;
    uint16_t inQuotes : 1;
};

struct SJsonParseInfo
{
    uint8_t inQuotes : 1;
    uint8_t foundParam : 1;
    EJsonParseSteps step : 2;
};

class CJsonProgmem
{
public:
    const char* addr = nullptr;
    char cache[4];
public:
    char getChar(const char* ptr) const
    {
        if (ptr >= addr && ptr < addr + sizeof(cache))
        {
            //Serial.print(cache[ptr - addr]);
            return cache[ptr - addr];
        }
        memcpy_P(cache, (PGM_P)ptr, sizeof(cache));
        *(const char **)(&addr) = ptr;
        //Serial.print(cache[0]);
        return cache[0];
    }

    String getString(const char* ptr, int len) const
    {
        String outString;
        outString.reserve(len);
        for (int i = 0; i < len; i++)
        {
            outString.concat(' ');
        }

        memcpy_P(outString.begin(), (PGM_P)ptr, len);
        
        return outString;
    }

    bool isEqual(const char* ptr, int len, const char* sourcePtr) const
    {
        return (strncmp_P(sourcePtr, ptr, len) == 0);
    }

    int length(const char* ptr) const
    {
        return strlen_P((PGM_P)ptr);
    }
};


class CJsonMemory
{
public:
    char getChar(const char* ptr) const
    {
        return *ptr;
    }

    String getString(const char* ptr, int len) const
    {
        String outString;
        for (int i = 0; i < len; i++)
        {
            outString.concat(ptr[i]);
        }
        return outString;
    }

    bool isEqual(const char* ptr, int len, const char* sourcePtr) const
    {
        return (strncmp(ptr, sourcePtr, len) == 0);
    }

    int length(const char* ptr) const
    {
        return strlen(ptr);
    }
};



template <class T>
class CJsonParserLite
{
private:
    T reader;
    const char* jsonText;
    int len;
private:
    void skipTo(const char*& str, const char* strMax, char toChar) const
    {
        SJsonSkipInfo skipInfo = {};
        char lastC = 0;
        while (str < strMax)
        {
            char c = reader.getChar(str);
            str++;

            if (skipInfo.inQuotes)
            {
                if (c == '"' && lastC != '\\')
                {
                    skipInfo.inQuotes = 0;
                }
            }
            else
            {
                if (c == '"')
                {
                    if (lastC != '\\')
                    {
                        skipInfo.inQuotes = 1;
                        continue;
                    }
                }

                if (c == toChar)
                {
                    if (skipInfo.arrayDepth == 0 || skipInfo.objectsDepth == 0)
                    {
                        return;
                    }
                }

                if (c == '[')
                {
                    skipInfo.arrayDepth++;
                }
                else if (c == ']')
                {
                    skipInfo.arrayDepth--;
                }
                else if (c == '{')
                {
                    skipInfo.objectsDepth++;
                }
                else if (c == '}')
                {
                    skipInfo.objectsDepth--;
                }
            }
            lastC = c;
        }
    }

    void trimValue(const char*& value, int& len) const
    {
        if (value != nullptr)
        {
            char c = reader.getChar(value);
            while ((c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '"') && len > 0)
            {
                value++;
                len--;
                c = reader.getChar(value);
            }

            const char* lv = value + len - 1;
            c = reader.getChar(lv);
            while ((c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '"') && len > 0)
            {
                lv--;
                len--;
                c = reader.getChar(lv);
            }
        }
    }
public:
    CJsonParserLite(const char* jsonText_, int len_ = -1) : jsonText(jsonText_), len(len_)
    {
        if (len < 0)
        {
            len = reader.length(jsonText);
        }
        trimValue(jsonText, len);
    }

    operator bool() const
    {
        return (jsonText != nullptr);
    }

    bool isObject() const
    {
        if (isValid() && len > 0)
        {
            char c = reader.getChar(jsonText);
            if (c == '{')
            {
                return true;
            }
        }
        return false;
    }

    bool isArray() const
    {
        if (isValid() && len > 0)
        {
            return (reader.getChar(jsonText) == '[');
        }
        return false;
    }

    bool isValid() const
    {
        return (jsonText != nullptr);
    }

    void getArrayItems(void(*func)(const CJsonParserLite<T>& item)) const
    {
        if (!isArray() || func == nullptr)
        {
            return;
        }

        SJsonParseInfo parseInfo = {};
        const char* str = jsonText + 1; // + begin array char [
        const char* strMax = jsonText + len;
        const char* currentText = str;
        char lastC = 0;
        int currentLen = 0;

        while (str < strMax)
        {
            char c = reader.getChar(str);
            str++;

            if (parseInfo.inQuotes)
            {
                if (c == '"' && lastC != '\\')
                {
                    parseInfo.inQuotes = 0;
                }
            }
            else
            {
                switch (c)
                {
                    case '{':
                        skipTo(str, strMax, '}');
                        currentLen = str - currentText;
                        func(CJsonParserLite<T>(currentText, currentLen));
                        parseInfo.foundParam = 0;
                        break;
                    case '[':
                        skipTo(str, strMax, '}');
                        currentLen = str - currentText;
                        func(CJsonParserLite<T>(currentText, currentLen));
                        parseInfo.foundParam = 0;
                        break;
                    case ']':
                        if (parseInfo.foundParam)
                        {
                            currentLen = (str - 1) - currentText;
                            func(CJsonParserLite<T>(currentText, currentLen));
                        }
                        return;
                    case ',':
                        currentText = str;
                        parseInfo.foundParam = 1;
                        break;
                }
            }

            lastC = c;
        }

    }

    void getObjectItems(void(*func)(const String& propName, const CJsonParserLite<T>& value)) const
    {
        if (!isObject() || func == nullptr)
        {
            return;
        }

        const char* str = jsonText + 1; // + begin object char {
        const char* strMax = jsonText + len;
        char lastC = 0;
        SJsonParseInfo parseInfo = {};
        const char* currentText = str;
        String propName;
        int currentLen = 0;

        while (str < strMax)
        {
            char c = reader.getChar(str);
            str++;

            if (parseInfo.inQuotes)
            {
                if (c == '"' && lastC != '\\')
                {
                    if (parseInfo.step == EJsonParseStep_ParamName)
                    {
                        currentLen = (str - 1) - currentText;
                        propName = reader.getString(currentText, currentLen);
                        parseInfo.foundParam = 1;
                    }
                    parseInfo.inQuotes = 0;
                }
            }
            else
            {
                switch (c)
                {
                    case '"':
                        if (lastC != c)
                        {
                            if (parseInfo.step == EJsonParseStep_None)
                            {
                                parseInfo.step = EJsonParseStep_ParamName;
                            }
                            currentText = str;
                            parseInfo.inQuotes = 1;
                        }
                        break;
                    case '{':
                        skipTo(str, strMax, '}');
                        currentLen = str - currentText;
                        func(propName, CJsonParserLite<T>(currentText, currentLen));
                        parseInfo.foundParam = 0;
                        break;
                    case '[':
                        skipTo(str, strMax, ']');
                        currentLen = str - currentText;
                        func(propName, CJsonParserLite<T>(currentText, currentLen));
                        parseInfo.foundParam = 0;
                        break;
                    case '}':
                        if (parseInfo.foundParam)
                        {
                            currentLen = (str - 1) - currentText;
                            func(propName, CJsonParserLite<T>(currentText, currentLen));
                        }
                        return;
                        break;
                    case ',':
                        currentLen = (str - 1) - currentText;
                        func(propName, CJsonParserLite<T>(currentText, currentLen));
                        parseInfo.foundParam = 0;
                        parseInfo.step = EJsonParseStep_None;
                        break;
                    case ':':
                        parseInfo.step = EJsonParseStep_ParamValue;
                        currentText = str;
                        break;
                    default:
                        break;
                }
            }

            lastC = c;
        }
    }

    CJsonParserLite<T> getProp(const __FlashStringHelper* propName) const
    {
        return getProp(String(propName).c_str());
    }

    CJsonParserLite<T> getProp(const char* propName) const
    {
        if (!isObject() || propName == nullptr || *propName == 0)
        {
            return CJsonParserLite<T>(nullptr, 0);
        }

        const char* str = jsonText + 1; // + begin object char {
        const char* strMax = jsonText + len;
        char lastC = 0;
        SJsonParseInfo parseInfo = {};
        const char* currentText = str;
        int propLen = strlen(propName);

        while (str < strMax)
        {
            char c = reader.getChar(str);
            str++;

            if (parseInfo.inQuotes)
            {
                if (c == '"' && lastC != '\\')
                {
                    if (parseInfo.step == EJsonParseStep_ParamName)
                    {
                        int currentLen = (str - 1) - currentText;
                        if (propLen == currentLen)
                        {
                            if (reader.isEqual(currentText, currentLen, propName))
                            {
                                parseInfo.foundParam = 1;
                            }
                        }
                    }
                    parseInfo.inQuotes = 0;
                }
            }
            else
            {
                switch (c)
                {
                    case '"':
                        if (lastC != c)
                        {
                            if (parseInfo.step == EJsonParseStep_None)
                            {
                                parseInfo.step = EJsonParseStep_ParamName;
                            }
                            currentText = str;
                            parseInfo.inQuotes = 1;
                        }
                        break;
                    case '{':
                        skipTo(str, strMax, '}');
                        if (parseInfo.foundParam)
                        {
                            int currentLen = str - currentText;
                            return CJsonParserLite<T>(currentText, currentLen);
                        }
                        break;
                    case '[':
                        skipTo(str, strMax, ']');
                        if (parseInfo.foundParam)
                        {
                            int currentLen = str - currentText;
                            return CJsonParserLite<T>(currentText, currentLen);
                        }
                        break;
                    case '}':
                        if (parseInfo.foundParam)
                        {
                            int currentLen = (str - 1) - currentText;
                            return CJsonParserLite<T>(currentText, currentLen);
                        }
                        return CJsonParserLite<T>(nullptr, 0);
                        break;
                    case ',':
                        if (parseInfo.foundParam)
                        {
                            int currentLen = (str - 1) - currentText;
                            return CJsonParserLite<T>(currentText, currentLen);
                        }
                        parseInfo.step = EJsonParseStep_None;
                        break;
                    case ':':
                        parseInfo.step = EJsonParseStep_ParamValue;
                        currentText = str;
                        break;
                    default:
                        break;
                }
            }

            lastC = c;
        }
        return CJsonParserLite<T>(nullptr, 0);
    }

    const char* toJson() const
    {
        return jsonText;
    }

    String toString() const
    {
        if (!isValid())
        {
            return String();
        }

        String outString;
        const char* str = jsonText;
        const char* strMax = str + len;
        char lastC = 0;

        while (str < strMax)
        {
            char c = reader.getChar(str);
            str++;

            if (c != '\\' && lastC != '\\')
            {
                outString.concat(c);
            }

            if (lastC == '\\')
            {
                switch (c)
                {
                    case 'b':
                        outString.concat(0x08);
                        break;
                    case 'f':
                        outString.concat(0x0F);
                        break;
                    case 'n':
                        outString.concat('\n');
                        break;
                    case 'r':
                        outString.concat('\r');
                        break;
                    case '\\':
                        outString.concat('\\');
                        break;
                    case 't':
                        outString.concat('\t');
                        break;
                }
            }

            lastC = c;
        }
        return outString;
    }
};

typedef CJsonParserLite<CJsonMemory> CJsonMemoryParserLite;
typedef CJsonParserLite<CJsonProgmem> CJsonProgmemParserLite;
