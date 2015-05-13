#!/bin/bash
#Test script for retrieve_token_expire_time method
echo "Enter token: "
read mytoken
curl -k -X GET "https://127.0.0.1/hydrogate/retrieve_token_expire_time?token=$mytoken"
echo ""
