#include <string>
#include <iostream>
#include <vector>
#include "stdint.h"
#include "ZipCodeRecord.h"


int main(){
    int zipCode = 40000;
    double latlong = 40.0;
    std::string place = "Test City";
    std::string state = "TS";
    std::string county = "Test County";

    std::cout << "Constructor Tests:\n";
    ZipCodeRecord a;
    ZipCodeRecord b(zipCode, latlong, latlong, place, state, county);
    std::cout << "Success! (Hopefully)\n\n";


    std::cout << "Getter Tests:\n";
    std::cout << "Get ZipCode: " << (zipCode == b.getZipCode()) << "\n";
    std::cout << "Get Latitude: " << (latlong == b.getLatitude()) << "\n";
    std::cout << "Get Longitude: " << (latlong == b.getLongitude()) << "\n";
    std::cout << "Get Location Name: " << (place == b.getLocationName()) << "\n";
    std::cout << "Get State Code: " << (state == b.getState()) << "\n";
    std::cout << "Get County Name: " << (county == b.getCounty()) << "\n";
    std::cout << "Success! (Hopefully)\n\n";


    std::cout << "Setter Tests:\n";
    std::cout << "Set ZipCode: " << (a.setZipCode(zipCode) && zipCode == a.getZipCode()) << "\n";
    std::cout << "Set ZipCode Fail Test (Under limit): " << !(a.setZipCode(-1)) << "\n";
    std::cout << "Set ZipCode Fail Test (Over limit): " << !(a.setZipCode(100000)) << "\n";
    std::cout << "Set Latitude: " << (a.setLatitude(latlong) && latlong == a.getLatitude()) << "\n";
    std::cout << "Set Latitude Fail Test (Under limit): " << !(a.setLatitude(-91.0)) << "\n";
    std::cout << "Set Latitude Fail Test (Over limit): " << !(a.setLatitude(91.0)) << "\n";
    std::cout << "Set Longitude: " << (a.setLongitude(latlong) && latlong == a.getLongitude()) << "\n";
    std::cout << "Set Longitude Fail Test (Under limit): " << !(a.setLongitude(-181.0)) << "\n";
    std::cout << "Set Longitude Fail Test (Over limit): " << !(a.setLongitude(181.0)) << "\n";
    std::cout << "Set Location Name: " << (a.setLocationName(place) && place == a.getLocationName()) << "\n";
    std::cout << "Set Location Name Fail Test (Too Short): " << !(a.setLocationName("")) << "\n";
    std::string toLong = "";
    for(int i = 0; i < 101; i++) toLong += " ";
    std::cout << "Set Location Name Fail Test (Too Long): " << !(a.setLocationName(toLong)) << "\n";
    std::cout << "Set State Code: " << (a.setState(state) && state == a.getState()) << "\n";
    std::cout << "Set State Code Fail Test (Too Short): " << !(a.setState("")) << "\n";
    std::cout << "Set State Code Fail Test (Too Long): " << !(a.setState("Too Long")) << "\n";
    std::cout << "Set County Name: " << (a.setCounty(county) && county == a.getCounty()) << "\n";
    std::cout << "Set County Name Fail Test (Too Short): " << !(a.setCounty("")) << "\n";
    toLong = "";
    for(int i = 0; i < 51; i++) toLong += " ";
    std::cout << "Set County Name Fail Test (Too Short): " << !(a.setCounty(toLong)) << "\n";
    std::cout << "Success! (Hopefully)\n\n";

    std::cout << "Direction Tests:\n";
    a.setLatitude(-90.0);
    b.setLatitude(90.0);
    std::cout << "Is North Of: " << (b.isNorthOf(a)) << "\n";
    std::cout << "Is Not North Of: " << !(a.isNorthOf(b)) << "\n";
    std::cout << "Is South Of: " << (a.isSouthOf(b)) << "\n";
    std::cout << "Is Not South Of: " << !(b.isSouthOf(a)) << "\n";
    a.setLongitude(-90.0);
    b.setLongitude(90.0);
    std::cout << "Is East Of: " << (b.isEastOf(a)) << "\n";
    std::cout << "Is Not East Of: " << !(a.isEastOf(b)) << "\n";
    std::cout << "Is West Of: " << (a.isWestOf(b)) << "\n";
    std::cout << "Is Not West Of: " << !(b.isWestOf(a)) << "\n";
    std::cout << "Success! (Hopefully)\n\n";

    std::cout << "Direction Tests:\n";
}