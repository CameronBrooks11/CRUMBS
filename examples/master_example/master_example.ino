/**
 * @file crumbs_master_test.ino
 * @brief CRUMBS Master example sketch to send messages to a CRUMBS Slice.
 */

#define CRUMBS_DEBUG
#include <CRUMBS.h>
#include <Wire.h>

// Instantiate CRUMBS as Master, set to true for Master mode
CRUMBS crumbsMaster(true); // Master mode

//  Maximum expected input length for serial commands.
#define MAX_INPUT_LENGTH 60

// Initializes the Master device, sets up serial communication, and provides usage instructions.
void setup()
{
    Serial.begin(115200); /**< Initialize serial communication at 115200 baud rate */

    while (!Serial)
    {
        delay(10); // Wait for Serial Monitor to open
    }

    crumbsMaster.begin(); /**< Initialize CRUMBS communication */

    printUsage(); /**< Print usage instructions */
}

void printUsage()
{
    Serial.println(F("Master ready. Enter messages in the format:"));
    Serial.println(F("address,typeID,commandType,data0,data1,data2,data3,data4,data5,errorFlags"));
    Serial.println(F("Example Serial Commands:"));
    Serial.println(F("0. Request Data Format Change:"));
    Serial.println(F("   8,1,0,1.0,2.0,3.0,4.0,5.0,6.0,0")); // Example: commandType 0 indicates a request to change the data format sent
    Serial.println(F("1. Send Command Message:"));
    Serial.println(F("   8,1,1,75.0,1.0,0.0,65.0,2.0,7.0,0")); // Example: address 8, typeID 1, commandType 1, data..., errorFlags 0
}

// Main loop that listens for serial input, parses commands, and sends CRUMBSMessages to the specified Slice.
void loop()
{
    // Listen for serial input to send commands or request data
    handleSerialInput();
}

/**
 * @brief Handles serial input from the user to send CRUMBSMessages to a specified target address.
 * @return none
 */
void handleSerialInput()
{
    if (Serial.available())
    {
        String input = Serial.readStringUntil('\n'); // Read input until newline
        input.trim();                                // Remove any extra whitespace

        Serial.print(F("Master: Received input from serial: "));
        Serial.println(input);

        // Check if the input starts with "request="
        if (input.startsWith("request="))
        {
            // Extract the address from the input string
            String addressString = input.substring(strlen("request="));
            addressString.trim();

            // Support hexadecimal format if it starts with "0x"
            uint8_t targetAddress = 0;
            if (addressString.startsWith("0x") || addressString.startsWith("0X"))
            {
                targetAddress = (uint8_t)strtol(addressString.c_str(), NULL, 16);
            }
            else
            {
                targetAddress = (uint8_t)addressString.toInt();
            }

            Serial.print(F("Master: Requesting data from address: 0x"));
            Serial.println(targetAddress, HEX);

            // Request CRUMBS_MESSAGE_SIZE bytes from the slave
            uint8_t numBytes = CRUMBS_MESSAGE_SIZE;
            Wire.requestFrom(targetAddress, numBytes);

            // Allow some time for the slave to send the response
            delay(50);

            uint8_t responseBuffer[CRUMBS_MESSAGE_SIZE];
            int index = 0;
            while (Wire.available() && index < CRUMBS_MESSAGE_SIZE)
            {
                responseBuffer[index++] = Wire.read();
            }

            Serial.print(F("Master: Received "));
            Serial.print(index);
            Serial.println(F(" bytes from slave."));

            // Attempt to decode the received response
            CRUMBSMessage response;
            if (crumbsMaster.decodeMessage(responseBuffer, index, response))
            {
                Serial.println(F("Master: Decoded response:"));
                Serial.print(F("typeID: "));
                Serial.println(response.typeID);
                Serial.print(F("commandType: "));
                Serial.println(response.commandType);
                Serial.print(F("data: "));
                for (int i = 0; i < 6; i++)
                {
                    Serial.print(response.data[i]);
                    Serial.print(F(" "));
                }
                Serial.println();
                Serial.print(F("errorFlags: "));
                Serial.println(response.errorFlags);
            }
            else
            {
                Serial.println(F("Master: Failed to decode response."));
            }

            return; // Exit the function after handling the request command.
        }

        // Otherwise, assume the input is a comma-separated message.
        uint8_t targetAddress;
        CRUMBSMessage message;
        bool parseSuccess = parseSerialInput(input, targetAddress, message);

        if (parseSuccess)
        {
            // Send the message to the specified target address
            crumbsMaster.sendMessage(message, targetAddress);
            Serial.println(F("Master: Message sent based on serial input."));
        }
        else
        {
            Serial.println(F("Master: Failed to parse serial input. Please check the format."));
        }
    }
}

/**
 * @brief Parses a comma-separated serial input string into a target address and CRUMBSMessage.
 *
 * @param input The input string from serial.
 * @param targetAddress Reference to store the parsed I2C address.
 * @param message Reference to store the parsed CRUMBSMessage.
 * @return true If parsing was successful.
 * @return false If parsing failed due to incorrect format or insufficient fields.
 */
bool parseSerialInput(const String &input, uint8_t &targetAddress, CRUMBSMessage &message)
{
    int fieldCount = 0; /**< Number of fields parsed */
    int lastComma = 0;  /**< Position of the last comma */

    // Initialize message fields to default values
    message.typeID = 0;
    message.commandType = 0;
    for (int i = 0; i < 6; i++)
    {
        message.data[i] = 0.0f;
    }
    message.errorFlags = 0;

    // Iterate through the input string to parse fields
    for (int i = 0; i <= input.length(); i++) // <= to capture the last field
    {
        if (i == input.length() || input[i] == ',')
        {
            String value = input.substring(lastComma, i);
            lastComma = i + 1;

            switch (fieldCount)
            {
            case 0:
                targetAddress = (uint8_t)value.toInt(); /**< Parse I2C address */
                break;
            case 1:
                message.typeID = (uint8_t)value.toInt(); /**< Parse typeID */
                break;
            case 2:
                message.commandType = (uint8_t)value.toInt(); /**< Parse commandType */
                break;
            case 3:
                message.data[0] = value.toFloat(); /**< Parse data0 */
                break;
            case 4:
                message.data[1] = value.toFloat(); /**< Parse data1 */
                break;
            case 5:
                message.data[2] = value.toFloat(); /**< Parse data2 */
                break;
            case 6:
                message.data[3] = value.toFloat(); /**< Parse data3 */
                break;
            case 7:
                message.data[4] = value.toFloat(); /**< Parse data4 */
                break;
            case 8:
                message.data[5] = value.toFloat(); /**< Parse data5 */
                break;
            case 9:
                message.errorFlags = (uint8_t)value.toInt(); /**< Parse errorFlags */
                break;
            default:
                // Extra fields are ignored
                break;
            }
            fieldCount++;
        }
    }

    // Check if the required number of fields are parsed
    if (fieldCount < 4)
    {
        Serial.println(F("Master: Not enough fields in input."));
        return false;
    }

    // Debugging output to verify parsed message
    Serial.println(F("Master: Parsed input into CRUMBSMessage and target address."));
    Serial.print(F("Target Address: "));
    Serial.println(targetAddress, HEX);
    Serial.print(F("Parsed Message -> typeID: "));
    Serial.print(message.typeID);
    Serial.print(F(", commandType: "));
    Serial.print(message.commandType);
    Serial.print(F(", data: "));
    for (int i = 0; i < 6; i++)
    {
        Serial.print(message.data[i]);
        Serial.print(F(" "));
    }
    Serial.print(F(", errorFlags: "));
    Serial.println(message.errorFlags);

    return true;
}
