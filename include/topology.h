/*
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Copyright (C) 2015 Intel Corporation
 *
 */

#ifndef __ALSA_TOPOLOGY_H
#define __ALSA_TOPOLOGY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \defgroup topology Topology Interface
 *  The topology interface.
 *  See \ref Topology page for more details.
 *  \{
 */

/*! \page topology ALSA Topology Interface
 *
 * ALSA Topology Interface
 *
 */

/* topology object type not used by kernel */
enum parser_type {
	PARSER_TYPE_TLV = 0,
	PARSER_TYPE_MIXER,
	PARSER_TYPE_ENUM,
	PARSER_TYPE_TEXT,
	PARSER_TYPE_DATA,
	PARSER_TYPE_BYTES,
	PARSER_TYPE_STREAM_CONFIG,
	PARSER_TYPE_STREAM_CAPS,
	PARSER_TYPE_PCM,
	PARSER_TYPE_DAPM_WIDGET,
	PARSER_TYPE_DAPM_GRAPH,
	PARSER_TYPE_BE,
	PARSER_TYPE_CC,
	PARSER_TYPE_MANIFEST,
};

typedef struct snd_tplg snd_tplg_t;

/**
 * \brief Create a new topology parser instance.
 * \return New topology parser instance
 */
snd_tplg_t *snd_tplg_new(void);

/**
 * \brief Free a topology parser instance.
 * \param tplg Topology parser instance
 */
void snd_tplg_free(snd_tplg_t *tplg);

/**
 * \brief Parse and build topology text file into binary file.
 * \param tplg Topology instance.
 * \param infile Topology text input file to be parsed
 * \param outfile Binary topology output file.
 * \return Zero on sucess, otherwise a negative error code
 */
int snd_tplg_build(snd_tplg_t *tplg, const char *infile, const char *outfile);

/**
 * \brief Enable verbose reporting of binary file output
 * \param tplg Topology Instance
 * \param verbose Enable verbose output if non zero
 */
void snd_tplg_verbose(snd_tplg_t *tplg, int verbose);

struct snd_tplg_widget_template {

};

struct snd_tplg_ctl_template {

};

struct snd_tplg_graph_elem {
	const char *src, *ctl, *sink;
};

struct snd_tplg_graph_template {
	int count;
	struct snd_tplg_graph_elem elem[0];
};

typedef struct snd_tplg_obj_template {
	enum parser_type type;
	int index;
	int version;		/* optional vendor specific version details */
	int vendor_type;	/* optional vendor specific type info */
	union {
		struct snd_tplg_widget_template *widget;
		struct snd_tplg_ctl_template *ctl;
		struct snd_tplg_graph_template *graph;
	};
};

int snd_tplg_add_object(snd_tplg_t *tplg, snd_tplg_obj_template *t);

#ifdef __cplusplus
}
#endif

#endif /* __ALSA_TOPOLOGY_H */
