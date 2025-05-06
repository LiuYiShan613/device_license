#include "aes256.hpp"
#include "aes_license.hpp"
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
#include <filesystem>
#include <exception>

LicenseChecker::LicenseChecker(const std::string& license_add, bool license_on){
    this->license_add = license_add;
    this->license_on = license_on;
}

std::string LicenseChecker::getSerialNumber() const {
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

// find "license-" haed name file
std::string LicenseChecker::getLicenseFileName() const {
    std::string directory = this->license_add; // set root
    std::string found_file;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find("license-") == 0) {
                found_file = filename;
                break; // if find one, exit
            }
        }
    }
    return found_file;
}

std::string LicenseChecker::readLicenseFromFile() const {
    //read license
    std::string read_license = getLicenseFileName();
    if (read_license.empty()) {
        return "no_key"; // No license file found
    }

    // return to original path 
    read_license = this->license_add + read_license ;
    
    try {
        std::ifstream file(read_license, std::ios::binary); 
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file.");
        }

        // Read prefix length
        uint32_t prefixLength;
        file.read(reinterpret_cast<char*>(&prefixLength), sizeof(prefixLength));
        std::string prefix(prefixLength, '\0');
        if (!file.read(&prefix[0], prefixLength)) {
            throw std::runtime_error("");
        }
        
        // Read encrypted Host name length
        uint32_t encryptedHostLength;
        file.read(reinterpret_cast<char*>(&encryptedHostLength), sizeof(encryptedHostLength));
        // Read encrypted Host name
        std::vector<unsigned char> encrypted_host(encryptedHostLength);
        if (!file.read(reinterpret_cast<char*>(&encrypted_host[0]), encryptedHostLength)) {
            throw std::runtime_error("");
        }

        // Read suffix length
        uint32_t suffixLength;
        file.read(reinterpret_cast<char*>(&suffixLength), sizeof(suffixLength));
        // Read suffix
        std::string suffix(suffixLength, '\0');
        if (!file.read(&suffix[0], suffixLength)) {
            throw std::runtime_error("");
        }

        // Decrypt encrypted Host name
        ByteArray key_b = {
            0x1a, 0xaf, 0xaa, 0x53, 0x2e, 0xd9, 0x45, 0x3d, 0x3f, 0x88, 0xb4, 0xe7, 0xce, 0xaf, 0x01, 0xeb, 
            0x18, 0x08, 0xa7, 0xf9, 0x0f, 0xb9, 0x2b, 0xe6, 0xcc, 0xf0, 0x39, 0xa2, 0x35, 0x14, 0x81, 0x1b
        };

        // decryption
        Aes256 aes(key_b);
        ByteArray decrypted_host;
        aes.decrypt_start(encrypted_host.size());
        aes.decrypt_continue(encrypted_host, decrypted_host);
        aes.decrypt_end(decrypted_host);

        std::string decryptedString(decrypted_host.begin(), decrypted_host.end());
        // Convert string into a stringstream
        std::stringstream ss(decryptedString);
        // store the split tokens
        std::vector<std::string> decryptedString_tokens;
        std::string token;
        // Split the string based on spaces
        while (std::getline(ss, token, ' ')) {
            decryptedString_tokens.push_back(token);
        }

        return decryptedString_tokens[1];

    } catch (const std::exception& e) {
        return "mistake";
    }
}

int LicenseChecker::isLicenseMatched() const {
    // Read the stored license from file
    std::string stored_license = readLicenseFromFile();
    // Calculate the license based on Serial Number
    std::string calculated_license = getSerialNumber();

    // to store the result
    int result = 0;
    
    if(this->license_on == false) result = 0;
    
    else{
        // license match
        if (stored_license == calculated_license)
            result = 0;
        // license exist but doesn't match
        else if (((stored_license != calculated_license)&&(stored_license != "no_key")) || (stored_license == "mistake"))
            result = 1;
        // license doesn't exist
        else if (stored_license == "no_key")
            result = 2;
    }

    return result;
}