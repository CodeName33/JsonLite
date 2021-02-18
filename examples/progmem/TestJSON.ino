#include <jsonlite.h>

char listOffset[16] = {};
char listOffsetIndex = 0;

void outConsole(const __FlashStringHelper *text, const char* text2 = nullptr)
{
    Serial.print(listOffset);
    Serial.print(text);
    if (text2 != nullptr)
    {
        Serial.print(text2);
    }
    Serial.println();
}

void listObject(const String &str, const CJsonProgmemParserLite& value);

void listArray(const CJsonProgmemParserLite& item)
{
    if (item.isArray())
    {
        outConsole(F("Array Begin"));
        listOffset[listOffsetIndex++] = '\t';
        item.getArrayItems(listArray);
        listOffset[listOffsetIndex--] = '\0';
        outConsole(F("Array End"));
    }

    if (item.isObject())
    {
        outConsole(F("Object Begin"));
        listOffset[listOffsetIndex++] = '\t';
        item.getObjectItems(listObject);
        listOffset[listOffsetIndex--] = '\0';
        outConsole(F("Object End"));
    }
    else
    {
        outConsole(F("Value = "), item.toString().c_str());
    }
}

void listObject(const String& str, const CJsonProgmemParserLite& value)
{
    outConsole(F("Property = "), str.c_str());
    if (value.isArray())
    {
        outConsole(F("Array Begin"));
        listOffset[listOffsetIndex++] = '\t';
        value.getArrayItems(listArray);
        listOffset[listOffsetIndex--] = '\0';
        outConsole(F("Array End"));
    }
    else if (value.isObject())
    {
        outConsole(F("Object Begin"));
        listOffset[listOffsetIndex++] = '\t';
        value.getObjectItems(listObject);
        listOffset[listOffsetIndex--] = '\0';
        outConsole(F("Object End"));
    }
    else
    {
        String v = value.toString();
        outConsole(F("Value = "), v.c_str());
    }
}

const char jsonData[] PROGMEM = "{\"effects\":[{\"text\": \"FFT\",\"params\": [{\"name\": \"color\" ,\"type\": \"list\" ,\"list\": [{ \"text\": \"Неон\" },{ \"text\": \"Синий\" },5]}]}]}";
        



void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("TEST 1:");
    uint32_t timeStart = millis();
    CJsonProgmemParserLite parser((const char*)jsonData);
    if (auto propEffects = parser.getProp(F("effects")))
    {
        propEffects.getArrayItems([](const CJsonProgmemParserLite& propEffect) {
            String text = propEffect.getProp(F("text")).toString();
            if (auto propParams = propEffect.getProp(F("params")))
            {
                propParams.getArrayItems([](const CJsonProgmemParserLite& propParam) {
                    String paramName = propParam.getProp(F("name")).toString();
                    String paramType = propParam.getProp(F("type")).toString();
                    if (auto propList = propParam.getProp(F("list")))
                    {
                        propList.getArrayItems([](const CJsonProgmemParserLite& propListItem) {
                            String value;
                            if (propListItem.isObject())
                            {
                                value = propListItem.getProp(F("text")).toString();
                            }
                            else
                            {
                                value = propListItem.toString();
                            }
                            Serial.print("item = ");
                            Serial.println(value);
                            });
                    }
                    });
                int KKk = 0;
            }
            });
        int kkk = 0;
    }
    Serial.print("Done in ");
    Serial.print(millis() - timeStart);
    Serial.println("ms");
    

    Serial.println("TEST 2:");

    timeStart = millis();
    if (parser.isArray())
    {
        outConsole(F("Array Begin"));
        listOffset[listOffsetIndex++] = '\t';
        parser.getArrayItems(listArray);
        listOffset[listOffsetIndex--] = '\0';
        outConsole(F("Array End"));
    }

    if (parser.isObject())
    {
        outConsole(F("Object Begin"));
        listOffset[listOffsetIndex++] = '\t';
        parser.getObjectItems(listObject);
        listOffset[listOffsetIndex--] = '\0';
        outConsole(F("Object End"));
    }

    Serial.print("Done in ");
    Serial.print(millis() - timeStart);
    Serial.println("ms");
}

void loop() {
  // put your main code here, to run repeatedly:

}
