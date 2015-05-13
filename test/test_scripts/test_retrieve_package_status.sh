#!/bin/bash
#Test script for retrieve_package_status method
echo "Enter token: "
read mytoken
echo "Enter package id: "
read mypackageid
curl -k -X GET "https://127.0.0.1/hydrogate/retrieve_package_status?token=$mytoken&packageid=$mypackageid"
echo ""
