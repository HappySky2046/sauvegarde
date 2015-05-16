/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *    packing.h
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
 * @file packing.h
 *
 * This file contains all the definitions of the functions and structures
 * to pack and unpack messages for all the programs of "Sauvegarde"
 * project.
 */
#ifndef _PACKING_H_
#define _PACKING_H_

/**
 * @def JANSSON_SUCCESS
 * Defines jansson success answer
 */
#define JANSSON_SUCCESS (0)


/**
 * @def ENC_NOT_FOUND
 * Indicates that something is wrong :
 *  - msg_id was not found into the JSON string.
 */
#define ENC_NOT_FOUND (0)


/**
 * @def ENC_META_DATA
 * Indicates that the encapsulated data is a meta_data_t * variable.
 */
#define ENC_META_DATA (1)


/**
 * @def IS_SERVEUR_UP
 * Indicates that we want to know if the serveur server is up.
 */
#define IS_SERVEUR_UP (2)


/**
 * @def ENC_END
 * Indicates that this is the end and that nothing else with occur after !
 * 127 is the end of the 7 bits ASCII table ;-)
 */
#define ENC_END (127)


/**
 * @struct capsule_t
 * @brief This structure encapsulate some commands and data that has
 *        to be transmited to antememoire's storing thread.
 */
typedef struct
{
    gint command; /**< Is an integer that says what to do based on ENC_*
                   *   macros
                   */
    void *data;   /**< Is a pointer to some structure. Type of the structure
                   *   is determined by the command parameter
                   */
} capsule_t;


/**
 * This function loads a JSON string into a json_t struture
 * @param json_str is the json string
 * @returns a pointer to a filled json_t * structure or NULL upon error
 */
extern json_t *load_json(gchar *json_str);


/**
 * Inserts a json_t *value into the json_t *root array.
 * @param[in,out] root is the root that will contain all meta data values
 * @param keyname is the keyname associated with the value (in fact it is
 *        variable's name)
 * @param value is the json_t "encoded" value to insert into the root
 */
void insert_json_value_into_json_root(json_t *root, gchar *keyname, json_t *value);


/**
 * gets a json_t *value into the json_t *root array.
 * @param[in,out] root is the root that contains all meta data values
 * @param keyname is the keyname associated with the value that we want to
 *        get back.
 * @returns the json_t "encoded" value from key keyname from the root
 */
extern json_t *get_json_value_from_json_root(json_t *root, gchar *keyname);


/**
 * returns the string with key keyname from the json tree root. It is used
 * by serveur to get the hostname from the json received message.
 * @param[in,out] root is the main json tree
 * @param keyname is the key for which we seek the string value.
 * @returns a newlly allocated gchar * string that is the value associated
 *          with key keyname. It can be freed with free_variable() when no longer
 *          needed.
 */
extern gchar *get_string_from_json_root(json_t *root, gchar *keyname);


/**
 * Converts the hash list to a json_t * array
 * @param hash_list : the GSList * list of hashs
 * @returns a json_t * array with the element of the list in it (if any).
 */
extern json_t *convert_hash_list_to_json(GSList *hash_list);


/**
 * Converts the file list to a json_t * array
 * @param file_list : the GSList * list of hashs
 * @returns a json_t * array with the element of the list in it (if any).
 */
extern json_t *convert_file_list_to_json(GSList *file_list);


/**
 * Converts the file list (a list of gchar *) to a json string
 * @param file_list : the GSList * list of hashs
 * @returns a gchar * string json formatted with all files (if any) in a
 *          json array
 */
extern gchar *convert_file_list_to_json_string(GSList *file_list);


/**
 * This function returns a list from an json array
 * @param root is the root json string that may contain an array named "name"
 * @param name is the name of the array to look for into
 * @returns a GSList that me be composed of 0 element (ie NULL).
 */
extern GSList *extract_gslist_from_array(json_t *root, gchar *name);


/**
 * Converts data with the associated hash to a json formatted string
 * @param a_data the data structure that contains the data whose checksum
 *               is a_hash
 * @param encoded_hash the hash of the data contained in a_data in base64
 *        encoded format.
 * @returns a json formatted string with those informations
 */
gchar *convert_data_to_json(data_t *a_data, gchar *encoded_hash);


/**
 * This function should return a JSON string with all informations from
 * the meta_data_t structure.
 * @param meta is the structure that contains all meta data for a file or
 *        a directory.
 * @param hostname is the name of the host onw hich we are running and that
 *        we want to include into the json string.
 * @returns a JSON formated string or NULL
 */
extern gchar *convert_meta_data_to_json(meta_data_t *meta, const gchar *hostname);


/**
 * Inserts a json formatted string into a hashs_t * balanced binary tree
 * structure and returns.
 * @param[in,out] hashs an initialized balanced binary tree structure
 *               (hashs_t *)
 * @param json_str a json string containing all data informations to be
 *        inserted into the tree.
 * @returns the base 64 encoded hash of the data beeing inserted.
 */
gchar *insert_json_into_hash_tree(hashs_t *hashs, gchar *json_str);


/**
 * Function that converts json_str containing the keys "hash", "data" and
 * "read" into hash
 */
hash_data_t *convert_json_to_hash_data(gchar *json_str);


/**
 * This function should return a newly allocated meta_data_t * structure
 * with all informations included from the json string.
 * @param json_str is a gchar * contianing the JSON formated string.
 * @returns a newly_allocated meta_data_t * structure that can be freed
 *          with free_meta_data_t() function when no longer needed. This
 *          function can return NULL if json_str is NULL itself.
 */
extern serveur_meta_data_t *convert_json_to_smeta_data(gchar *json_str);


/**
 * Function that encapsulate a meta_data_t * variable into a capsule_t *
 * one. It does not check that meta is not NULL so it may encapsulate a
 * NULL pointer !
 * @param command is the command to be used with the encapsulated data.
 * @param meta is the meta_data_t * variable to be encapsulated
 * @returns a capsule_t * with command field set to ENC_META_DATA stating
 *          that the data field is of type meta_data_t *.
 */
extern capsule_t *encapsulate_meta_data_t(gint command, meta_data_t *meta);


/**
 * Function that encapsulate an END command.
 * @returns a capsule_t * with command field set to ENC_END stating
 *          that this is the end my friend (some famous song) !
 */
extern capsule_t *encapsulate_end(void);


/**
 * This function returns the MESSAGE_ID from msg_id JSON field
 * @param json_str : a gchar * containing the JSON formated string.
 * @returns a gint that correspond to the msg_id field found in json_str.
 *          If the field is not found it returns ENC_NOT_FOUND. This field
 *          is based on ENC_* constants that are also used for the
 *          communication between threads in client
 */
extern gint get_json_message_id(gchar *json_str);


/**
 * Gets the version of a version json string as returned by serveur's
 * server.
 * @param json_str : a gchar * containing the JSON formated string.
 * @returns version string or NULL
 */
extern gchar *get_json_version(gchar *json_str);


/**
 * Converts to a json gchar * string. Used only by serveur's program
 * @param name : name of the program of which we want to print the version.
 * @param date : publication date of this version
 * @param version : version of the program.
 * @param authors : authors that contributed to this program
 * @param license : license in use for this program and its sources
 * @returns a newlly allocated gchar * string in json format that can be
 *          freed when no longer needed.
 */
extern gchar *convert_version_to_json(gchar *name, gchar *date, gchar *version, gchar *authors, gchar *license);
#endif /* #ifndef _PACKING_H_ */
