/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *    serveur.c
 *    This file is part of "Sauvegarde" project.
 *
 *    (C) Copyright 2014 - 2015 Olivier Delhomme
 *     e-mail : olivier.delhomme@free.fr
 *
 *    "Sauvegarde" is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    "Sauvegarde" is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with "Sauvegarde".  If not, see <http://www.gnu.org/licenses/>
 */

/**
 * @file serveur.c
 * This file contains all the stuff for the cdpfglserver program of "Sauvegarde"
 * project. The aim of this program is to save every checksum and data and
 * meta data of every 'client' program that is connected to.
 */

#include "serveur.h"

static void free_serveur_struct_t(serveur_struct_t *serveur_struct);
static gboolean int_signal_handler(gpointer user_data);
static serveur_struct_t *init_serveur_main_structure(int argc, char **argv);
static gchar *get_data_from_a_specific_hash(serveur_struct_t *serveur_struct, gchar *hash);
static gchar *get_argument_value_from_key(struct MHD_Connection *connection, gchar *key, gboolean encoded);
static gchar *get_a_list_of_files(serveur_struct_t *serveur_struct, struct MHD_Connection *connection);
static gchar *get_json_answer(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, const char *url);
static gchar *get_unformatted_answer(serveur_struct_t *serveur_struct, const char *url);
static int process_get_request(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, const char *url, void **con_cls);
static int answer_meta_json_post_request(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, gchar *received_data);
static int process_received_data(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, const char *url, gchar *received_data);
static int process_post_request(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, const char *url, void **con_cls, const char *upload_data, size_t *upload_data_size);
static int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
static void print_headers(struct MHD_Connection *connection);
static int ahc(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls);
static gpointer meta_data_thread(gpointer user_data);
static gpointer data_thread(gpointer user_data);


/**
 * Frees server's structure
 * @param serveur_struct is the structure to be freed
 */
void free_serveur_struct_t(serveur_struct_t *serveur_struct)
{

    if (serveur_struct != NULL)
        {
            MHD_stop_daemon(serveur_struct->d);
            free_variable(serveur_struct->backend); /** we need a backend function to be called to free th backend structure */
            g_thread_unref(serveur_struct->data_thread);
            g_thread_unref(serveur_struct->meta_thread);
            free_options_t_structure(serveur_struct->opt);
            free_variable(serveur_struct);
        }
}


/**
 * SIGINT signal handler function
 * @param user_data is a gpointer that MUST be a pointer to the
 *        serveur_struct_t *
 * @returns FALSE if user_data is NULL and frees memory and exits if TRUE.
 */
static gboolean int_signal_handler(gpointer user_data)
{
    serveur_struct_t *serveur_struct = (serveur_struct_t *) user_data;

    if (serveur_struct != NULL)
        {
            print_debug(_("\nEnding the program.\n"));
            g_main_loop_quit(serveur_struct->loop);
            free_serveur_struct_t(serveur_struct);
        }

    /** we can remove the handler as we are exiting the program anyway */
    return FALSE;
}


/**
 * Inits main server's structure
 * @param argc : number of arguments given on the command line.
 * @param argv : an array of strings that contains command line arguments.
 * @returns a serveur_struct_t * structure that contains everything that is
 *          needed for 'cdpfglserver' program.
 */
static serveur_struct_t *init_serveur_main_structure(int argc, char **argv)
{
    serveur_struct_t *serveur_struct = NULL;  /** main structure for 'serveur' program. */

    serveur_struct = (serveur_struct_t *) g_malloc0(sizeof(serveur_struct_t));

    serveur_struct->data_thread = NULL;
    serveur_struct->meta_thread = NULL;
    serveur_struct->opt = do_what_is_needed_from_command_line_options(argc, argv);
    serveur_struct->d = NULL;            /* libmicrohttpd daemon pointer */
    serveur_struct->meta_queue = g_async_queue_new();
    serveur_struct->data_queue = g_async_queue_new();
    serveur_struct->loop = NULL;

    /* default backend (file_backend) */
    serveur_struct->backend = init_backend_structure(file_store_smeta, file_store_data, file_init_backend, file_build_needed_hash_list, file_get_list_of_files, file_retrieve_data);

    return serveur_struct;
}


/**
 * Function that gets the data of a specific hash
 * @param serveur_struct is the main structure for the server.
 * @param hash is the hash (in hex format) of which we want the data.
 * @returns a json formatted string.
 */
static gchar *get_data_from_a_specific_hash(serveur_struct_t *serveur_struct, gchar *hash)
{
    gchar *answer = NULL;
    backend_t *backend = NULL;
    hash_data_t *hash_data = NULL;

    if (serveur_struct != NULL && serveur_struct->backend != NULL)
        {
            backend = serveur_struct->backend;

            if (backend->retrieve_data != NULL)
                {
                    hash_data = backend->retrieve_data(serveur_struct, hash);
                    answer = convert_hash_data_t_to_string(hash_data);
                    free_hash_data_t_structure(hash_data);

                    if (answer == NULL)
                        {
                            answer = g_strdup_printf("Error while trying to get data from hash %s", hash);
                        }
                }
            else
                {
                    answer = g_strdup(_("This backend's missing a retrieve_data function!"));
                }
        }
    else
        {
            answer = g_strdup(_("Something's wrong with server's initialisation!"));
        }

    return answer;
}


/**
 * Function that gets the argument corresponding to the key 'key' in the
 * url (from connection)
 * @param connection is the connection in MHD
 * @param key the key to look for into the url
 * @param encoded is a boolean that is TRUE if value is base64 encoded
 * @returns a gchar * string that may be freed when no longer needed
 */
static gchar *get_argument_value_from_key(struct MHD_Connection *connection, gchar *key, gboolean encoded)
{
    const char *value = NULL;
    gchar *value_dup = NULL;
    gsize len = 0;

    if (connection != NULL && key != NULL)
        {
            value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, key);

            if (value != NULL)
                {
                    if (encoded == TRUE)
                        {
                            value_dup = (gchar *) g_base64_decode(value, &len);
                        }
                    else
                        {
                            value_dup = g_strdup(value);
                        }
                }
        }

    return value_dup;
}


/**
 * Function to get a list of saved files.
 * @param serveur_struct is the main structure for the server.
 * @param connection is the connection in MHD
 * @param url is the requested url
 * @returns a json formatted string or NULL
 */
static gchar *get_a_list_of_files(serveur_struct_t *serveur_struct, struct MHD_Connection *connection)
{
    gchar *answer = NULL;
    gchar *hostname = NULL;
    gchar *uid = NULL;
    gchar *gid = NULL;
    gchar *owner = NULL;
    gchar *group = NULL;
    gchar *filename = NULL;
    gchar *date = NULL;
    backend_t *backend = NULL;
    query_t *query = NULL;


    if (serveur_struct != NULL && serveur_struct->backend != NULL)
        {
            backend = serveur_struct->backend;

            if (backend->get_list_of_files != NULL)
                {
                    hostname = get_argument_value_from_key(connection, "hostname", FALSE);
                    uid = get_argument_value_from_key(connection, "uid", FALSE);
                    gid = get_argument_value_from_key(connection, "gid", FALSE);
                    owner = get_argument_value_from_key(connection, "owner", FALSE);
                    group = get_argument_value_from_key(connection, "group", FALSE);
                    filename = get_argument_value_from_key(connection, "filename", TRUE);
                    date = get_argument_value_from_key(connection, "date", TRUE);

                    print_debug(_("hostname: %s, uid: %s, gid: %s, owner: %s, group: %s, filter: %s && %s\n"), hostname, uid, gid, owner, group, filename, date);

                    if (hostname != NULL && uid != NULL && gid != NULL && owner != NULL && group != NULL)
                        {
                            query = init_query_structure(hostname, uid, gid, owner, group, filename, date);
                            answer = backend->get_list_of_files(serveur_struct, query);
                            free_query_structure(query); /** All variables hostname, uid... are freed there ! */
                        }
                    else
                        {
                            answer = g_strdup_printf(_("Malformed request. hostname: %s, uid: %s, gid: %s, owner: %s, group: %s"), hostname, uid, gid, owner, group);
                            free_variable(hostname);
                            free_variable(uid);
                            free_variable(gid);
                            free_variable(owner);
                            free_variable(group);
                            free_variable(filename);
                            free_variable(date);
                        }
                }
        }

    return answer;
}


/**
 * Function to answer to get requests in a json way. This mode should be
 * prefered.
 * @param serveur_struct is the main structure for the server.
 * @param connection is the connection in MHD
 * @param url is the requested url
 * @note to translators all json requests MUST NOT be translated because
 *       it is the protocol itself !
 * @returns a newlly allocated gchar * string that contains the anwser to be
 *          sent back to the client.
 */
static gchar *get_json_answer(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, const char *url)
{
    gchar *answer = NULL;
    gchar *hash = NULL;
    size_t hlen = 0;

    if (g_strcmp0(url, "/Version.json") == 0)
        {
            answer = convert_version_to_json(PROGRAM_NAME, SERVEUR_DATE, SERVEUR_VERSION, SERVEUR_AUTHORS, SERVEUR_LICENSE);
        }
    else if (g_str_has_prefix(url, "/File/List.json"))
        {
            answer = get_a_list_of_files(serveur_struct, connection);
        }
    else if (g_str_has_prefix(url, "/Data/"))
        {
            hash = g_strndup((const gchar *) url + 6, HASH_LEN*2);  /* HASH_LEN is expressed when hash is in binary form  */
            hash = g_strcanon(hash, "abcdef0123456789", '\0');      /* replace anything not in hexadecimal format with \0 */

            hlen = strlen(hash);
            if (hlen == HASH_LEN*2)
                {
                    print_debug(_("Trying to get data for hash %s\n"), hash);
                    answer = get_data_from_a_specific_hash(serveur_struct, hash);
                }
            else
                {
                    answer = g_strdup_printf("{\"Invalid url: in %s hash has length\": %zd instead of %d}", url, hlen, HASH_LEN*2);
                }

            free_variable(hash);
        }
    else
        { /* Some sort of echo to the invalid request */
            answer = g_strdup_printf("{\"Invalid url\": %s}", url);
        }

    return answer;
}


/**
 * Function to answer to get requests in an unformatted way. Only some urls
 * May be like this. As we prefer to speak in json format in normal operation
 * mode
 * @param serveur_struct is the main structure for the server.
 * @param url is the requested url
 * @returns a newlly allocated gchar * string that contains the anwser to be
 *          sent back to the client.
 */
static gchar *get_unformatted_answer(serveur_struct_t *serveur_struct, const char *url)
{
    gchar *answer = NULL;
    gchar *buf1 = NULL;
    gchar *buf2 = NULL;
    gchar *buf3 = NULL;

    if (g_strcmp0(url, "/Version") == 0)
        {
            buf1 = buffer_program_version(PROGRAM_NAME, SERVEUR_DATE, SERVEUR_VERSION, SERVEUR_AUTHORS, SERVEUR_LICENSE);
            buf2 = buffer_libraries_versions(PROGRAM_NAME);
            buf3 = buffer_selected_option(serveur_struct->opt);

            answer = g_strconcat(buf1, buf2, buf3, NULL);

            buf1 = free_variable(buf1);
            buf2 = free_variable(buf2);
            buf3 = free_variable(buf3);
        }
    else
        { /* Some sort of echo to the invalid request */
            answer = g_strdup_printf(_("Error: invalid url: %s\n"), url);
        }

    return answer;
}


/**
 * Creates a response sent to the client via MHD_queue_response
 * @param connection is the MHD_Connection connection
 * @param answer is the gchar * string to be sent.
 */
static int create_MHD_response(struct MHD_Connection *connection, gchar *answer)
{
    struct MHD_Response *response = NULL;
    int success = MHD_NO;
    a_clock_t *elapsed = NULL;

    elapsed = new_clock_t();
    response = MHD_create_response_from_buffer(strlen(answer), (void *) answer, MHD_RESPMEM_MUST_FREE);
    success = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    end_clock(elapsed, "creation MHD response");

    return success;
}


/**
 * Function to process get requests received from clients.
 * @param serveur_struct is the main structure for the server.
 * @param connection is the connection in MHD
 * @param url is the requested url
 * @param con_cls is a pointer used to know if this is the first call or not
 * @returns an int that is either MHD_NO or MHD_YES upon failure or not.
 */
static int process_get_request(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, const char *url, void **con_cls)
{
    static int aptr = 0;
    int success = MHD_NO;
    gchar *answer = NULL;


    if (&aptr != *con_cls)
        {
            /* do never respond on first call */
            *con_cls = &aptr;

            success = MHD_YES;
        }
    else
        {
            if (get_debug_mode() == TRUE)
                {
                    print_debug(_("Requested get url: %s\n"), url);
                    print_headers(connection);
                }

            if (g_str_has_suffix(url, ".json"))
                { /* A json format answer was requested */
                    answer = get_json_answer(serveur_struct, connection, url);
                }
            else
                { /* An "unformatted" answer was requested */
                    answer = get_unformatted_answer(serveur_struct, url);
                }

                /* reset when done */
                *con_cls = NULL;

                /* Do not free answer variable as MHD will do it for us ! */
                success = create_MHD_response(connection, answer);
        }

    return success;

}


/**
 * Answers /Meta.json POST request by storing data and answering to the
 * client.
 * @param serveur_struct is the main structure for the server.
 * @param connection is the connection in MHD
 * @param received_data is a gchar * string to the data that was received
 *        by the POST request.
 */
static int answer_meta_json_post_request(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, gchar *received_data)
{
    serveur_meta_data_t *smeta = NULL;
    gchar *answer = NULL;       /** gchar *answer : Do not free answer variable as MHD will do it for us !  */
    json_t *root = NULL;        /** json_t *root is the root that will contain all meta data json formatted */
    json_t *array = NULL;       /** json_t *array is the array that will receive base64 encoded hashs       */
    GSList *needed = NULL;

    smeta = convert_json_to_smeta_data(received_data);

    if (smeta != NULL && smeta->meta != NULL)
        {   /* The convertion went well and smeta contains the meta data */

            print_debug(_("Received meta data for file %s\n"), smeta->meta->name);

            if (smeta->data_sent == FALSE)
                {
                    /**
                     * Creating an answer and sending the hashs that are needed. If
                     * the selected backend does not have a build_needed_hash_list
                     * function we are returning the whole hash_list !
                     */

                    if (serveur_struct != NULL && serveur_struct->backend != NULL && serveur_struct->backend->build_needed_hash_list != NULL)
                        {
                            needed = serveur_struct->backend->build_needed_hash_list(serveur_struct, smeta->meta->hash_data_list);
                            array = convert_hash_list_to_json(needed);
                            needed = free_list(needed);
                        }
                    else
                        {
                            array = convert_hash_list_to_json(smeta->meta->hash_data_list);
                        }
                }
            else
                {
                    array = json_array();
                }

            root = json_object();
            insert_json_value_into_json_root(root, "hash_list", array);
            answer = json_dumps(root, 0);
            json_decref(root);


            /**
             * Sending smeta data into the queue in order to be treated by
             * the corresponding thread. smeta is freed there and should not
             * be used after this "call" here.
             */
            g_async_queue_push(serveur_struct->meta_queue, smeta);
        }
    else
        {
            answer = g_strdup_printf(_("Error: could not convert json to metadata\n"));
        }

    return create_MHD_response(connection, answer);
}


/**
 * Function that process the received data from the POST command and
 * answers to the client.
 * Here we may do something with this data (we may want to store it
 * somewhere).
 *
 * @param serveur_struct is the main structure for the server.
 * @param connection is the connection in MHD
 * @param url is the requested url
 * @param received_data is a gchar * string to the data that was received
 *        by the POST request.
 */
static int process_received_data(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, const char *url, gchar *received_data)
{
    gchar *answer = NULL;                   /** gchar *answer : Do not free answer variable as MHD will do it for us ! */
    int success = MHD_NO;
    gchar *encoded_hash = NULL;
    hash_data_t *hash_data = NULL;
    json_t *root = NULL;
    GSList *hash_data_list = NULL;
    GSList *head = NULL;
    a_clock_t *elapsed = NULL;

    if (g_strcmp0(url, "/Meta.json") == 0 && received_data != NULL)
        {
            success = answer_meta_json_post_request(serveur_struct, connection, received_data);
        }
    else if (g_strcmp0(url, "/Data.json") == 0 && received_data != NULL)
        {

            hash_data = convert_string_to_hash_data(received_data);

            encoded_hash = g_base64_encode(hash_data->hash, HASH_LEN);
            print_debug(_("Received data for hash: \"%s\" (%ld bytes)\n"), encoded_hash, hash_data->read);
            free_variable(encoded_hash);

            /**
             * Sending received_data into the queue in order to be treated by
             * the corresponding thread. hash_data is freed by data_thread
             * and should not be used after this "call" here.
             */
            g_async_queue_push(serveur_struct->data_queue, hash_data);


            /**
             * creating an answer for the client to say that everything went Ok!
             */
            answer = g_strdup_printf(_("Ok!"));
            success = create_MHD_response(connection, answer);
        }
    else if (g_strcmp0(url, "/Data_Array.json") == 0 && received_data != NULL)
        {
            /* print_debug("/Data_Array.json: %s\n", received_data); */
            elapsed = new_clock_t();
            root = load_json(received_data);
            hash_data_list = extract_gslist_from_array(root, "data_array", FALSE);
            head = hash_data_list;
            json_decref(root);
            end_clock(elapsed, "extract_gslist_from_array");

            elapsed = new_clock_t();
            while (hash_data_list != NULL)
                {
                    hash_data = hash_data_list->data;

                    /* Only for debbugging ! */
                    encoded_hash = g_base64_encode(hash_data->hash, HASH_LEN);
                    print_debug(_("Received data for hash: \"%s\" (%ld bytes)\n"), encoded_hash, hash_data->read);
                    free_variable(encoded_hash);

                    /** Sending hash_data into the queue. */
                    g_async_queue_push(serveur_struct->data_queue, hash_data);
                    hash_data_list = g_slist_next(hash_data_list);
                }

            g_slist_free(head);
            end_clock(elapsed, "push hash_data to queue");

            /**
             * creating an answer for the client to say that everything went Ok!
             */

            answer = g_strdup_printf(_("Ok!"));
            success = create_MHD_response(connection, answer);
        }
    else
        {
            /* The url is unknown to the server and we can not process the request ! */
            print_error(__FILE__, __LINE__, "Error: invalid url: %s\n", url);
            answer = g_strdup_printf(_("Error: invalid url!\n"));
            success = create_MHD_response(connection, answer);
        }

    return success;
}


/**
 * Function to process post requests.
 * @param serveur_struct is the main structure for the server.
 * @param connection is the connection in MHD
 * @param url is the requested url
 * @param con_cls is a pointer used to know if this is the first call or not
 * @param upload_data is a char * pointer to the data being uploaded at this call
 * @param upload_size is a pointer to an size_t value that says how many data
 *        is ti be copied from upload_data string.
 * @returns an int that is either MHD_NO or MHD_YES upon failure or not.
 */
static int process_post_request(serveur_struct_t *serveur_struct, struct MHD_Connection *connection, const char *url, void **con_cls, const char *upload_data, size_t *upload_data_size)
{
    int success = MHD_NO;
    gchar *pp = *con_cls;
    gchar *newpp = NULL;
    gchar *buf1 = NULL;
    const char *length = NULL;

    /* print_debug("%ld, %s, %p\n", *upload_data_size, url, pp); */ /* This is for early debug only ! */

    if (pp == NULL)
        {
            /* print_headers(connection); */
            length = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Content-Length");
            fprintf(stderr, "length = %s\n", length);
            /* Initialzing the structure at first connection */
            pp = g_strdup("");
            *con_cls = pp;

            success = MHD_YES;
        }
    else if (*upload_data_size != 0)
        {
            /* Getting data whatever they are */
            buf1 = g_strndup(upload_data, *upload_data_size);
            newpp = g_strconcat(pp, buf1, NULL);
            buf1 = free_variable(buf1);
            pp = free_variable(pp);

            *con_cls = newpp;

            *upload_data_size = 0;

            success = MHD_YES;
        }
    else
        {
            /* reset when done */
            *con_cls = NULL;

            /* Do something with received_data */
            success = process_received_data(serveur_struct, connection, url, pp);

            free_variable(pp);
        }

    return success;
}


/**
 * Prints all keys-values pairs contained in an HTTP header.
 * @param cls cls may be NULL here (not used).
 * @param[in] kind is the MHD_ValueKind requested
 * @param[in] key is the key to be printed
 * @param[in] value is the value of the corresponding key
 *
 */
static int print_out_key(void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
  fprintf(stdout, "\t%s: %s\n", key, value);

  return MHD_YES;
}


/**
 * Prints everything in the header of a connection
 * @param connection the connection that we want to print all headers.
 */
static void print_headers(struct MHD_Connection *connection)
{
    fprintf(stdout, _("Headers for this connection are:\n"));
    MHD_get_connection_values(connection, MHD_HEADER_KIND, &print_out_key, NULL);
}


/**
 * MHD_AccessHandlerCallback function that manages all connections requests
 * @param cls is the serveur_struct_t * serveur_struct main serveur
 *        structure.
 * @todo . free some memory where needed
 *       . manage errors codes
 */
static int ahc(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    serveur_struct_t *ahc_serveur_struct = (serveur_struct_t *) cls;
    int success = MHD_NO;


    if (g_strcmp0(method, "GET") == 0)
        {
            /* We have a GET method that needs to be processed */
            success = process_get_request(ahc_serveur_struct, connection, url, con_cls);
        }
    else if (g_strcmp0(method, "POST") == 0)
        {  /* We have a POST method that needs to be processed */
            success = process_post_request(ahc_serveur_struct, connection, url, con_cls, upload_data, upload_data_size);
        }
    else
        { /* not a GET nor a POST -> we do not know what to do ! */
            success = MHD_NO;
        }

    return success;
}


/**
 * Thread whose aim is to store meta-data according to the selected backend
 * @param data : serveur_struct_t * structure.
 * @returns NULL to fullfill the template needed to create a GThread
 */
static gpointer meta_data_thread(gpointer user_data)
{
    serveur_struct_t *serveur_struct = user_data;
    serveur_meta_data_t *smeta = NULL;

    if (serveur_struct != NULL && serveur_struct->meta_queue != NULL)
        {

            if (serveur_struct->backend != NULL && serveur_struct->backend->store_smeta != NULL)
                {

                    while (TRUE)
                        {
                            smeta = g_async_queue_pop(serveur_struct->meta_queue);

                            if (smeta != NULL)
                                {
                                    print_debug("meta_data_thread: received from %s meta for file %s\n", smeta->hostname, smeta->meta->name);
                                    serveur_struct->backend->store_smeta(serveur_struct, smeta);
                                    free_smeta_data_t(smeta);
                                }
                            else
                                {
                                    print_error(__FILE__, __LINE__, _("Error: received a NULL pointer.\n"));
                                }
                        }
                }
            else
                {
                    print_error(__FILE__, __LINE__, _("Error: no meta data store backend defined, meta-data's thread terminating...\n"));
                }
        }
    else
        {
            print_error(__FILE__, __LINE__, _("Error: unable to launch meta-data thread.\n"));
        }

    return NULL;
}


/**
 * Thread whose aim is to store data according to the selected backend
 * @param data : serveur_struct_t * structure.
 * @returns NULL to fullfill the template needed to create a GThread
 */
static gpointer data_thread(gpointer user_data)
{
    serveur_struct_t *dt_serveur_struct = user_data;
    hash_data_t *hash_data = NULL;

    if (dt_serveur_struct != NULL && dt_serveur_struct->meta_queue != NULL)
        {

            if (dt_serveur_struct->backend != NULL && dt_serveur_struct->backend->store_data != NULL)
                {

                    while (TRUE)
                        {
                            hash_data = g_async_queue_pop(dt_serveur_struct->data_queue);

                            if (hash_data != NULL)
                                {
                                    dt_serveur_struct->backend->store_data(dt_serveur_struct, hash_data);
                                }
                        }
                }
            else
                {
                    print_error(__FILE__, __LINE__, _("Error: no data store backend defined, data's thread terminating...\n"));
                }

        }
    else
        {
            print_error(__FILE__, __LINE__, _("Error while trying to launch data thread"));
        }

    return NULL;
}


/**
 * Main function
 * @param argc : number of arguments given on the command line.
 * @param argv : an array of strings that contains command line arguments.
 * @returns always 0
 */
int main(int argc, char **argv)
{
    serveur_struct_t *serveur_struct = NULL;  /** main structure for 'serveur' program.           */
    guint id_int = 0;
    guint id_term = 0;

    #if !GLIB_CHECK_VERSION(2, 36, 0)
        g_type_init();  /** g_type_init() is deprecated since glib 2.36 */
    #endif

    ignore_sigpipe(); /** into order to get libmicrohttpd portable */

    init_international_languages();

    serveur_struct = init_serveur_main_structure(argc, argv);

    if (serveur_struct != NULL && serveur_struct->opt != NULL && serveur_struct->backend != NULL)
        {
            serveur_struct->loop = g_main_loop_new(g_main_context_default(), FALSE);
            id_int = g_unix_signal_add(SIGINT, int_signal_handler, serveur_struct);
            id_term = g_unix_signal_add(SIGTERM, int_signal_handler, serveur_struct);

            if (id_int <= 0 || id_term <= 0)
                {
                    print_error(__FILE__, __LINE__, _("Unable to add signal handler\n"));
                }

            /* Initializing the choosen backend by calling it's function */
            if (serveur_struct->backend->init_backend != NULL)
                {
                   serveur_struct->backend->init_backend(serveur_struct);
                }

            /* Before starting anything else, start the threads */
            serveur_struct->meta_thread = g_thread_new("meta-data", meta_data_thread, serveur_struct);
            serveur_struct->data_thread = g_thread_new("data", data_thread, serveur_struct);

            /* Starting the libmicrohttpd daemon */
            serveur_struct->d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG, serveur_struct->opt->port, NULL, NULL, &ahc, serveur_struct, MHD_OPTION_CONNECTION_MEMORY_LIMIT, (size_t) 131070 , MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120, MHD_OPTION_END);

            if (serveur_struct->d == NULL)
                {
                    print_error(__FILE__, __LINE__, _("Error while spawning libmicrohttpd daemon\n"));
                    return 1;
                }

            /* Unless on error we will never join the threads as they
             * contain a while (TRUE) loop !
             */
            g_main_loop_run(serveur_struct->loop);

            /* g_thread_join(serveur_struct->meta_thread); */
            /* g_thread_join(serveur_struct->data_thread); */


        }
    else
        {
            print_error(__FILE__, __LINE__, _("Error: initialization failed.\n"));
        }

    return 0;
}
