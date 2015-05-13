#!/bin/bash
##Test script for request_token method
curl -k -X POST --data "username=root&password=rootpass" https://127.0.0.1/hydrogate/request_token/
echo ""
