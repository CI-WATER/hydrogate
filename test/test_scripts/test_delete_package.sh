#!/bin/bash
#Test script for delete_package method
echo "Enter token: "
read mytoken
echo "Enter package id: "
read mypackageid
curl -k -X DELETE "https://127.0.0.1/hydrogate/delete_package?token=$mytoken&packageid=$mypackageid"
echo ""
