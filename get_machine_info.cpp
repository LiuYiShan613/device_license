#include "aes256.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <fstream>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include <random>
#include <regex>

std::string getSerialNumber() {
    std::string serialnumber;
    // get serial number
    FILE *pipe = popen("sudo dmidecode -t system | grep 'Serial Number'", "r");
    if (!pipe) {
        std::cerr << "Error: Failed to run command." << std::endl;
        return "";
    }
    char buffer[128];
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            serialnumber += buffer;
    }
    pclose(pipe);

    // Find the position of "Serial Number:"
    size_t pos = serialnumber.find("Serial Number:");
    if (pos != std::string::npos) {
        // Extract substring after "Serial Number:"
        serialnumber = serialnumber.substr(pos + std::string("Serial Number:").length());
    }
    // Trim leading whitespace
    serialnumber.erase(0, serialnumber.find_first_not_of(" \t\n\r\f\v"));
    // Trim trailing whitespace
    serialnumber.erase(serialnumber.find_last_not_of(" \t\n\r\f\v") + 1);

    return serialnumber;

}

std::string getHostName() {
    std::string hostname;
    FILE *pipe = popen("hostname", "r");
    if (!pipe) {
        std::cerr << "Error: Failed to run command." << std::endl;
        return "";
    }
    char buffer[128];
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            hostname += buffer;
    }
    pclose(pipe);

    // Remove trailing newline character
    if (!hostname.empty() && hostname[hostname.size() - 1] == '\n') {
        hostname.erase(hostname.size() - 1);
    }

    // to generate file name (machine-info-xxxx.bin)
    std::string host = "";
    // find first dot position, to address host name like twtp1pcxxxx.deltaos.corp
    size_t dotPosition = hostname.find('.'); 
    if (dotPosition != std::string::npos) {
        // to get string before first dot
        std::string beforeDot = hostname.substr(0, dotPosition);
        host = beforeDot;
    } else {
        host = hostname;
    }
    std::string hostfile_name = "machine-info-" + host + ".bin";

    return hostfile_name;
}

std::string generateRandomString(size_t length) {
    const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dist(0, charset.size() - 1);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(gen)];
    }
    return result;
}

int main() {

    // Key for encryption/decryption (must be 32 bytes)
    ByteArray key_a = {
        0xfd, 0x46, 0x80, 0xe2, 0x28, 0xd7, 0xa7, 0x71, 0x66, 0x22, 0x4a, 0xa5, 0x5b, 0x62, 0x13, 0xb7, 
        0xdd, 0x60, 0xcd, 0xec, 0x02, 0xb1, 0x02, 0x6a, 0x90, 0x28, 0x69, 0x1b, 0x7c, 0x64, 0x2a, 0x10
    };

    // Create an instance of AES256
    Aes256 aes(key_a);
    std::string serialnumber = getSerialNumber();

    // trouble test
    // serialnumber = "000000000000";
    // std::cout<< serialnumber << std::endl;

    if (serialnumber.empty()) {
        std::cerr << "Failed to get Serial Number" << std::endl;
        return EXIT_FAILURE;
    }

    // Generate random strings for prefix and suffix
    std::string prefix = generateRandomString(100);
    std::string suffix = generateRandomString(100);

    // Combine Host name with prefix and suffix
    std::string combinedString = prefix + " " + serialnumber + " " + suffix;

    // Convert combined string to byte array
    std::vector<unsigned char> combinedBytes(combinedString.begin(), combinedString.end());

    // Encryption
    ByteArray encrypted_host;
    aes.encrypt_start(combinedBytes.size(), encrypted_host);
    aes.encrypt_continue(combinedBytes, encrypted_host);
    aes.encrypt_end(encrypted_host);

    // get hostfile name to save bin file";
    std::string hostfile_name = getHostName();

    // Output encrypted Host name, prefix and suffix to a binary file
    std::ofstream outfile(hostfile_name, std::ios::binary);
    if (outfile.is_open()) {
        // Write prefix length
        uint32_t prefixLength = static_cast<uint32_t>(prefix.size());
        outfile.write(reinterpret_cast<const char*>(&prefixLength), sizeof(prefixLength));
        // Write prefix
        outfile.write(prefix.c_str(), prefix.size());

        // Write encrypted Host name length
        uint32_t encryptedHostLength = static_cast<uint32_t>(encrypted_host.size());
        outfile.write(reinterpret_cast<const char*>(&encryptedHostLength), sizeof(encryptedHostLength));
        // Write encrypted Host name
        outfile.write(reinterpret_cast<const char*>(&encrypted_host[0]), encrypted_host.size());

        // Write suffix length
        uint32_t suffixLength = static_cast<uint32_t>(suffix.size());
        outfile.write(reinterpret_cast<const char*>(&suffixLength), sizeof(suffixLength));
        // Write suffix
        outfile.write(suffix.c_str(), suffix.size());

        outfile.close();
        std::cout << "Please send the machine info to us." << std::endl;

    } else {
        std::cerr << "Unable to open file for writing!" << std::endl;
        return 1;
    }

    return 0;
}
