/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 *    options.h
 *    This file is part of "Sauvegarde" project.
 *
 *    (C) Copyright 2014 - 2017 Olivier Delhomme
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
 * @file client/options.h
 *
 *  This file contains all the definitions for the options of the command
 *  line.
 */
#ifndef _OPTIONS_H_
#define _OPTIONS_H_

/**
 * @struct options_t
 * @brief Selected options.
 *
 * Structure Options gives a way to store program options passed from the
 * command line or read from a configuration file.
 */
typedef struct
{
    gboolean version;     /**< TRUE if we have to display program's version                                           */
    GSList *dirname_list; /**< Directory names that were left in the command line                                     */
    GSList *exclude_list; /**< List of all files and directories to be excluded from beeing saved.                    */
    gint64 blocksize;     /**< block size in bytes                                                                    */
    gchar *configfile;    /**< filename for the configuration file specified on the command line                      */
    gchar *dircache;      /**< Directory where we will cache files and temporary stuff to do the job                  */
    gchar *dbname;        /**< File name of the database that is used to cache files and buffers localy               */
    gchar *ip;            /**< A string representing the IP address where server is located (may be a hotsname)       */
    gint port;            /**< Port number on which to send things to server's server (on which it must listen)       */
    gint buffersize;      /**< buffersize is an option to choose how many bytes we may accumulate before sending them */
    gboolean adaptive;    /**< adaptive will make client compute hashs with an adaptive blocksize if TRUE             */
    gboolean noscan;      /**< noscan will avoid the first directory scan when set to TRUE. default = FALSE           */
    gshort cmptype;       /**< compression type to be used when communicating. See compress.h for available types     */
} options_t;


/**
 * This function parses command line options.
 * @param argc : number of arguments given on the command line.
 * @param argv : an array of strings that contains command line arguments.
 */
extern options_t *manage_command_line_options(int argc, char **argv);


/**
 * Frees the options structure if necessary
 * @param opt : the malloc'ed options_t structure
 */
extern void free_options_t(options_t *opt);


/**
 * Decides what to do upon command lines options passed to the program
 * @param argc : number of arguments given on the command line.
 * @param argv : an array of strings that contains command line arguments.
 * @returns options_t structure malloc'ed and filled upon choosen command
 *          line's option (in manage_command_line_options function).
 */
extern options_t *do_what_is_needed_from_command_line_options(int argc, char **argv);


#endif /* #IFNDEF _OPTIONS_H_ */
