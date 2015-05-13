#!/bin/bash
#Test script for test_return_hpc_names method
echo "Enter token: "
read mytoken
curl -k -X GET "https://127.0.0.1/hydrogate/return_hpc_names?token=$mytoken"
echo ""
