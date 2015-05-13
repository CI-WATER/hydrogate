#!/bin/bash
#Test script for delete_job method
echo "Enter token: "
read mytoken
echo "Enter job id: "
read myjobid
curl -k -X DELETE "https://127.0.0.1/hydrogate/delete_job?token=$mytoken&jobid=$myjobid"
echo ""
