# API

All urls ending with .json waits (POST) or provide (GET) json strings (the upcoming v0.0.11 will also output errors in json strings).


## GET

### /Version

Gets the version of the server in a more human readable than a json string
producing an output such as:

    cdpfglserver version: 0.0.1-cb6c92b (27 04 2014)
    Author(s): Olivier DELHOMME <olivier.delhomme@free.fr>
    License: GPL v3 or later

    cdpfglserver was compiled with the following libraries:
        . GLIB version: 2.36.3
        . LIBMHD: 0.9.39-2
        . SQLITE version: 3.8.6
        . JANSSON version: 2.7.0

    cdpfglserver options are:
    Configuration file: /home/dup/local/bin/../etc/sauvegarde/server.conf
    Port number: 5468


### /Version.json

Gets the version of the server in a json parsable string producing an
output such as:

    {"revision": "cb6c92b",
     "name": "cdpfglserver",
     "date": "27 04 2014",
     "librairies": [{"glib": "2.36.3"},
                    {"mhd": "0.9.39-2"},
                    {"sqlite": "3.8.6"},
                    {"jansson": "2.7.0"}
                   ],
     "authors": ["Olivier DELHOMME <olivier.delhomme@free.fr>"],
     "version": "0.0.1",
     "licence": "GPL v3 or later"
    }


### /File/List.json

Gets a list of all saved files and their versions. This url needs five
parameters: hostname, uid, gid, owner and group. An url must include at
least all theses parameters:

    http://192.168.0.152:5468/File/List.json?hostname=julia&uid=530&gid=530&owner=dup&group=admin"

'filename' may be used to filter results for a specific filename (or
directory name).It must be base 64 encoded.

'date', 'afterdate' and 'beforedate' may be used to filter results for 
a specific date. They must be base64 encoded and the string must be 
formatted "YYYY-MM-DD HH:MM:SS".

The answer contains a json array named "file_list" that contains a
server_meta_data_t structure (a JSON structure for a file as stated in
[infrastructure.md](infrastructure.md) file) for each file.


### /Data/beeff34162c5402270369ad624c15bdc8f599df220b7d540239b774eabb57fb0.json

Gets the associated data of the hash that has to be transmitted in
hexadecimal format.


### /Data/Hash_Array.json

Gets the associated data of the hash list that MUST be transmitted into
the GET command ```X-Get-Hash-Array``` header that must contain a comma
separated base64 encoded hash list. Associated data is a JSON string with
three fields, hash, data and size. hash field contains a random base64
encoded hash that is not reliable. data is all hash corresponding blocks
data concatenated and base64 encoded. size is the real size of the data
(not encoded).


## POST

### /Meta.json

Waits for a json string with "hash_list", "filetype", "group", "mode",
"owner", "atime", "uid", "ctime", "name", "mtime", "gid", "fsize", "hostname",
"data_sent" meta-data fields and a "msg_id" field in no specific order.
"hash_list" field should contains hashs of data in an ordered way and
base64 encoded.

The server answers a hash_list that may be empty. This hash list is the
hashs that the server needs. The server should not ask for hash that it
already has. Example:

    {"hash_list": ["QKlVxnrQi/A338uHJUSKw3C+RfmJ2KUdiZPMru/Z9kE=",
                   ...,
                   "nZjIbSp6tJHAQJ71rzVt/KGoTeX4BXDLYvXx/PQM5j8="]
    }

"data_sent" is true when data has already been sent before sending
metadata and false otherwise. If "data_sent" is true then server does
not tries to know which hashs are needed because it assumes that every
block of needed data has already been received (thus no block is needed
anymore).


### /Data.json

Waits for a json string with "hash", "data" and "size" fields. hash
and data fields must be base 64 encoded.

    {"hash": "QKlVxnrQi/A338uHJUSKw3C+RfmJ2KUdiZPMru/Z9kE=",
     "data": "8oBc08li ... WW9mr20u4=",
     "size": 12140
    }


### /Data_Array.json

Waits for a json string containing an array named "data_array". This array
contains a suite of json strings (at least two) each of them containing
"hash", "data" and "size" fields as for /Data.json.


### /Hash_Array.json

Waits for a json string containing an array named "hash_list". This array
contains a suite of json strings that represents hashs of blocks to be
transmited to the server. The server must answer to the client a json
string containing an array named "hash_list" with a suite of hashs that
are needed (server's unknown hashs).

