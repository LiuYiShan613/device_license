# How to use
#### step 1.
- `get_machine_info.cpp`
  - To generate `machine information (machine-info-xxxx.bin)` from users.  
  ```
  g++ get_machine_info.cpp aes256.cpp -o machine_info
  ```
  - To get machine information(machine-info-xxxx.bin) .  
  ```
  ./machine_info
  ```
- `license.cpp`
  - To generate `license(license-xxxx.bin)` based on machine information(machine-info-xxxx.bin)for verification.  
  ```
  g++ license.cpp aes256.cpp -o license
  ```
  - To get license(license-xxxx.bin) to run DIVA .  
  ```
  ./license
  ```

#### step 2.
- modify start_conatiner.sh if run in container (pass if run locally)
  - add --network=host to sudo docker run
- modify setup_DIVA_license.py
  - `aes256.cpp, aes_license.cpp`add these two files to compile with DIVA code                                        
  - take DIVA Object detection as example           
  ```python 
  ext_modules = [
        Pybind11Extension( "DIVA_obj",
            sources = [ 'DIVA_cpp_src_lib/src/DIVA_obj.cpp', 'sdk/aes_license.cpp', 'sdk/aes256.cpp' ],
            #....................
        ),
  ]
  ```

#### step 3.
- `LicenseChecker::isLicenseMatched()`
  - Please put this method into your program, it will check whether your Serial number is correct.
  - take Object detection as example           
  ```c++
  #include "sdk/aes_license.hpp"

  class ObjectDetectionsWithMotionvector
  {
    // init
    std::string file_address = "c_pack/sdk/"; // set license root
    bool license_on = true; // license on(true) or off(false) 
    LicenseChecker checker = LicenseChecker(file_address, license_on);

    ObjectDetectionsWithMotionvector(
      int img_width,
      int img_height,
      int macroblock_size = 16,
      bool newobj = true,
      bool debug = false)
    {

      if (checker.isLicenseMatched()==0) {
          std::cout << "License match. " << std::endl;
          // exit(0);
      }
      else if(checker.isLicenseMatched()==1){
          std::cout << "License exist but doesn't match. " << std::endl;
          exit(0);
      }
      else if(checker.isLicenseMatched()==2){
          std::cout << "License doesn't exist, check if placed License in the correct location. " << std::endl;
          exit(0);
      }
          //.....................//
    }
  }
  ```
