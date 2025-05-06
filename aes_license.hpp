#ifndef LICENSECHECKER_HPP
#define LICENSECHECKER_HPP

#include <string>

class LicenseChecker {
    private:
        std::string getSerialNumber() const;
        std::string getLicenseFileName() const;
        std::string readLicenseFromFile() const;
    public:
        std::string license_add;
        bool license_on;
        LicenseChecker(const std::string& license_add, bool license_on);
        int isLicenseMatched() const;
    
};

#endif /* LICENSECHECKER_HPP */
