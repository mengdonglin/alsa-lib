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

#include <sound/asoc.h>

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
int snd_tplg_build_file(snd_tplg_t *tplg, const char *infile,
	const char *outfile);

/**
 * \brief Enable verbose reporting of binary file output
 * \param tplg Topology Instance
 * \param verbose Enable verbose output if non zero
 */
void snd_tplg_verbose(snd_tplg_t *tplg, int verbose);


struct snd_tplg_tlv_template {
	int size;	/* in bytes aligned to 4 */
	int numid;	/* control element numeric identification */
	int count;	/* number of elem in data array */
	unsigned int data[SND_SOC_TPLG_TLV_SIZE];
};

struct snd_tplg_channel_elem {
	int size;	/* in bytes of this structure */
	int reg;
	int shift;
	int id;	/* ID maps to Left, Right, LFE etc */
};

struct snd_tplg_channel_map_template {
	int num_channels;
	struct snd_tplg_channel_elem channel[SND_SOC_TPLG_MAX_CHAN];
};

struct snd_tplg_pdata_template {
	unsigned int length;
	const void *data;
};

struct snd_tplg_ctl_ops_template {
	int get;
	int put;
	int info;
};

struct snd_tplg_ctl_template {
	int type;
	const char *name;
	int access;
	struct snd_soc_tplg_kcontrol_ops_id ops;
	struct snd_tplg_tlv_template *tlv; /* non NULL means we have TLV data */
};

struct snd_tplg_mixer_template {
	struct snd_tplg_ctl_template hdr;
	struct snd_tplg_channel_map_template *map;
	int min;
	int max;
	int platform_max;
	int invert;
};

struct snd_tplg_enum_template {
	struct snd_tplg_ctl_template hdr;
	struct snd_tplg_channel_map_template *map;
	int items;
	int mask;
	int count;
	const char **texts;
	const int **values;
};

struct snd_tplg_bytes_template {
	struct snd_tplg_ctl_template hdr;
	int max;
	int mask;
	int base;
	int num_regs;
};

struct snd_tplg_graph_elem {
	const char *src, *ctl, *sink;
};

struct snd_tplg_graph_template {
	int count;
	struct snd_tplg_graph_elem elem[0];
};

struct snd_tplg_widget_template {
	int id;		/* SND_SOC_DAPM_CTL */
	const char *name;
	const char *sname;
	int reg;		/* negative reg = no direct dapm */
	int shift;		/* bits to shift */
	int mask;		/* non-shifted mask */
	int subseq;		/* sort within widget type */
	unsigned int invert;		/* invert the power bit */
	unsigned int ignore_suspend;	/* kept enabled over suspend */
	unsigned short event_flags;
	unsigned short event_type;
	unsigned short num_kcontrols;
	struct snd_soc_tplg_private *priv;
	int num_ctls;
	struct snd_tplg_ctl_template *ctl[];
};

typedef struct snd_tplg_obj_template {
	enum parser_type type;
	int index;
	int version;		/* optional vendor specific version details */
	int vendor_type;	/* optional vendor specific type info */
	union {
		struct snd_tplg_widget_template *widget;
		struct snd_tplg_mixer_template *mixer;
		struct snd_tplg_graph_template *graph;
	};
} snd_tplg_obj_template_t;

int snd_tplg_add_object(snd_tplg_t *tplg, snd_tplg_obj_template_t *t);

int snd_tplg_build(snd_tplg_t *tplg, const char *outfile);

int snd_tplp_set_manifest_data(snd_tplg_t *tplg, const void *data, int len);

#ifdef __cplusplus
}
#endif

#endif /* __ALSA_TOPOLOGY_H */
