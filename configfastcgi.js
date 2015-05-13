    {    
       "service" : {  
        	"api" : "fastcgi",  
        	"socket": "stdin" // we use socket given by server  
       },
       "cache" : {
                "backend" : "process_shared",
                "memory" : 65536,
       },
       "security" : {
                "aeskey" : "fae3kkkkdla11437idzx",
                "usersalt1" : 1234567891,
                "usersalt2" : 9876543211,
                "tokenexpiredurationinseconds": 500,
       },
       
       "database" : {
                "connectionstring" : "user=postgres password=postgrespass dbname=hpcdb hostaddr=127.0.0.1 port=5432",
       },
       
       "callback" : {
                "packagecallbackurl" : "http://127.0.0.1?packageid=$id&state=$state",
                "packagecallbackenabled" : false,
                "jobcallbackurl" : "http://127.0.0.1?jobid=$id&state=$state",
                "jobcallbackenabled" : false,
                "isabsolutecallbackurl" : false,
       },
       
       "scheduler" : {
                "workeritemcount" : 50,
                "maxsshconnectionperhpc" : 10,
                "scpbufsiz" : 102400,
                "stdoutbufsiz" : 512,
                "jobstatuscheckdelayinseconds" : 10,
                "isinputinlocalmode" : false,
                "isoutputinlocalmode" : false,
                "isabsoluteurl" : true,
                "inputfolder" : "../data/",
                "outputfolder" : "../data/",
                "inputurl" : "https://127.0.0.1/$file",
                "outputurlsrc" : "/irods/home/rods/",
                "outputurl" : "sftp://sftpusername:sftppass@127.0.0.1/~/hydrogatedata/",
       }
    } 
