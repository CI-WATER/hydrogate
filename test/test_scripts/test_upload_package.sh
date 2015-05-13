#!/bin/bash
#Test script for upload_package method
echo "Enter token: "
read mytoken
echo "Enter package url"
read packageurl
curl -k -X POST --data "token=$mytoken&package=$packageurl&hpc=HydrogateHPC" https://127.0.0.1/hydrogate/upload_package/
echo ""
