#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <random>
#include <iomanip>
#include <dirent.h>
#include <cstring>
#include <sys/stat.h>

#include "aes256.hpp"

std::string generateRandomString(size_t length) {
    const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()_+-=[]{}|;:,.<>?";
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dist(0, charset.size() - 1);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(gen)];
    }
    return result;
}

// Function to remove prefix and suffix
std::string strip_machine_info_and_bin(const std::string& filename) {
    const std::string prefix = "machine-info-";
    const std::string suffix = ".bin";
    
    // Check if the filename starts with the prefix and ends with the suffix
    if (filename.rfind(prefix, 0) == 0 && filename.size() > prefix.size() + suffix.size() && 
        filename.substr(filename.size() - suffix.size()) == suffix) {
        // Extract the substring between the prefix and suffix
        return filename.substr(prefix.size(), filename.size() - prefix.size() - suffix.size());
    } else {
        // If the filename does not match the expected format, return the original filename
        return filename;
    }
}

int main() {
    const char* directory_path = "."; 
    DIR* dir = opendir(directory_path);
    // to store the matching filename
    std::string read_machineinfo_file = "";
    struct dirent* entry;
    int file_count = 0; 

    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        // Check if the filename starts with "machine-info" and ends with ".bin"
        if (filename.rfind("machine-info", 0) == 0 && filename.size() > 4 && filename.substr(filename.size() - 4) == ".bin") {
            file_count++;
            if (file_count > 1) {
                std::cerr << "More than one 'machine-info' bin file found. Only one file is allowed at a time." << std::endl;
                closedir(dir);
                exit(0); 
            }
            read_machineinfo_file = filename;
        }
    }
    
    closedir(dir); 

    if (file_count == 0) {
        std::cout << "No matching file found" << std::endl;
    } 

    // to check if machine-info available to open
    std::ifstream file(read_machineinfo_file, std::ios::binary); 
    if (!file.is_open()) {
        throw std::runtime_error("Error: Unable to open machine info bin file."); 
    }

    // Read prefix length
    uint32_t prefixLength;
    file.read(reinterpret_cast<char*>(&prefixLength), sizeof(prefixLength));
    // Read prefix
    std::string prefix(prefixLength, '\0');
    file.read(&prefix[0], prefixLength);

    // Read encrypted Serial Number length
    uint32_t encryptedSerialNoLength;
    file.read(reinterpret_cast<char*>(&encryptedSerialNoLength), sizeof(encryptedSerialNoLength));
    // Read encrypted Serial Number
    std::vector<unsigned char> encrypted_serialno(encryptedSerialNoLength);
    file.read(reinterpret_cast<char*>(&encrypted_serialno[0]), encryptedSerialNoLength);

    // Read suffix length
    uint32_t suffixLength;
    file.read(reinterpret_cast<char*>(&suffixLength), sizeof(suffixLength));
    // Read suffix
    std::string suffix(suffixLength, '\0');
    file.read(&suffix[0], suffixLength);

    // Decrypt encrypted Serial Number
    ByteArray key_a = {
        0xfd, 0x46, 0x80, 0xe2, 0x28, 0xd7, 0xa7, 0x71, 0x66, 0x22, 0x4a, 0xa5, 0x5b, 0x62, 0x13, 0xb7, 
        0xdd, 0x60, 0xcd, 0xec, 0x02, 0xb1, 0x02, 0x6a, 0x90, 0x28, 0x69, 0x1b, 0x7c, 0x64, 0x2a, 0x10
    };
    
    // decryption
    Aes256 aes(key_a);
    ByteArray decrypted_serialno;
    aes.decrypt_start(encrypted_serialno.size());
    aes.decrypt_continue(encrypted_serialno, decrypted_serialno);
    aes.decrypt_end(decrypted_serialno);

    std::string decryptedString(decrypted_serialno.begin(), decrypted_serialno.end());

    // Convert string into a stringstream
    std::stringstream ss(decryptedString);
    // store the split tokens
    std::vector<std::string> decryptedString_tokens;
    std::string token;
    // Split the string based on spaces
    while (std::getline(ss, token, ' ')) {
        decryptedString_tokens.push_back(token);
    }

    // Generate new random strings for prefix and suffix
    std::string prefix_new = generateRandomString(150);
    std::string suffix_new = generateRandomString(150);

    // Combine Serial Number with prefix and suffix
    std::string combinedString_new = prefix_new + " " + decryptedString_tokens[1] + " " + suffix_new;

    // Convert combined string to byte array
    std::vector<unsigned char> combinedBytes(combinedString_new.begin(), combinedString_new.end());

    // Encrypted Serial Number
    ByteArray key_b = {
        0x1a, 0xaf, 0xaa, 0x53, 0x2e, 0xd9, 0x45, 0x3d, 0x3f, 0x88, 0xb4, 0xe7, 0xce, 0xaf, 0x01, 0xeb, 
        0x18, 0x08, 0xa7, 0xf9, 0x0f, 0xb9, 0x2b, 0xe6, 0xcc, 0xf0, 0x39, 0xa2, 0x35, 0x14, 0x81, 0x1b
    };
    
    // Encryption
    Aes256 aes_b(key_b);
    ByteArray encrypted_serialno_new;
    aes_b.encrypt_start(combinedBytes.size(), encrypted_serialno_new);
    aes_b.encrypt_continue(combinedBytes, encrypted_serialno_new);
    aes_b.encrypt_end(encrypted_serialno_new);

    // to generate output_license file name (license-xxxx.bin)
    std::string host = strip_machine_info_and_bin(read_machineinfo_file);
    std::string output_license_file = "license-" + host + ".bin";

    // Output encrypted Serial Number, prefix and suffix to a binary file
    std::ofstream outfile(output_license_file, std::ios::binary);
    if (outfile.is_open()) {
        // Write prefix length
        uint32_t prefixLength = static_cast<uint32_t>(prefix_new.size());
        outfile.write(reinterpret_cast<const char*>(&prefixLength), sizeof(prefixLength));
        // Write prefix
        outfile.write(prefix_new.c_str(), prefix_new.size());

        // Write encrypted Serial Number length
        uint32_t encryptedSerialNoLength = static_cast<uint32_t>(encrypted_serialno_new.size());
        outfile.write(reinterpret_cast<const char*>(&encryptedSerialNoLength), sizeof(encryptedSerialNoLength));
        // Write encrypted Serial Number
        outfile.write(reinterpret_cast<const char*>(&encrypted_serialno_new[0]), encrypted_serialno_new.size());

        // Write suffix length
        uint32_t suffixLength = static_cast<uint32_t>(suffix_new.size());
        outfile.write(reinterpret_cast<const char*>(&suffixLength), sizeof(suffixLength));
        // Write suffix
        outfile.write(suffix_new.c_str(), suffix_new.size());

        outfile.close();
        std::cout << "Generate license success." << std::endl;

    } else {
        std::cerr << "Unable to open file for writing!" << std::endl;
        return 1;
    }

    return 0;
}

