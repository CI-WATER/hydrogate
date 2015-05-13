#!/bin/bash
#Test script for retrieve_program_info method
echo "Enter token: "
read mytoken
echo "Enter progran name: "
read programname;
curl -k -X GET "https://127.0.0.1/hydrogate/retrieve_program_info?token=$mytoken&program=$programname"
echo ""
