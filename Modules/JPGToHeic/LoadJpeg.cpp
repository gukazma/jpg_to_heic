#include "LoadJpeg.h"
extern "C" {
	// Prevent duplicate definition for libjpeg-turbo v2.0
	// Note: these 'undef's are only a workaround for a libjpeg-turbo-v2.0 bug and
	// should be removed again later. Bug has been fixed in libjpeg-turbo-v2.0.1.
#include <jconfig.h>
#if defined(LIBJPEG_TURBO_VERSION_NUMBER) && LIBJPEG_TURBO_VERSION_NUMBER == 2000000
#undef HAVE_STDDEF_H
#undef HAVE_STDLIB_H
#endif
#include <jpeglib.h>
}
#include <iostream>
#include <assert.h>
#define JPEG_ICC_MARKER  (JPEG_APP0+2)  /* JPEG marker code for ICC */
#define JPEG_ICC_OVERHEAD_LEN  14        /* size of non-profile data in APP2 */


static bool JPEGMarkerIsIcc(jpeg_saved_marker_ptr marker)
{
	return
		marker->marker == JPEG_ICC_MARKER &&
		marker->data_length >= JPEG_ICC_OVERHEAD_LEN &&
		/* verify the identifying string */
		GETJOCTET(marker->data[0]) == 0x49 &&
		GETJOCTET(marker->data[1]) == 0x43 &&
		GETJOCTET(marker->data[2]) == 0x43 &&
		GETJOCTET(marker->data[3]) == 0x5F &&
		GETJOCTET(marker->data[4]) == 0x50 &&
		GETJOCTET(marker->data[5]) == 0x52 &&
		GETJOCTET(marker->data[6]) == 0x4F &&
		GETJOCTET(marker->data[7]) == 0x46 &&
		GETJOCTET(marker->data[8]) == 0x49 &&
		GETJOCTET(marker->data[9]) == 0x4C &&
		GETJOCTET(marker->data[10]) == 0x45 &&
		GETJOCTET(marker->data[11]) == 0x0;
}

boolean ReadICCProfileFromJPEG(j_decompress_ptr cinfo,
	JOCTET** icc_data_ptr,
	unsigned int* icc_data_len)
{
	jpeg_saved_marker_ptr marker;
	int num_markers = 0;
	int seq_no;
	JOCTET* icc_data;
	unsigned int total_length;
#define MAX_SEQ_NO  255        /* sufficient since marker numbers are bytes */
	char marker_present[MAX_SEQ_NO + 1];      /* 1 if marker found */
	unsigned int data_length[MAX_SEQ_NO + 1]; /* size of profile data in marker */
	unsigned int data_offset[MAX_SEQ_NO + 1]; /* offset for data in marker */

	*icc_data_ptr = NULL;        /* avoid confusion if FALSE return */
	*icc_data_len = 0;

	/* This first pass over the saved markers discovers whether there are
	 * any ICC markers and verifies the consistency of the marker numbering.
	 */

	for (seq_no = 1; seq_no <= MAX_SEQ_NO; seq_no++)
		marker_present[seq_no] = 0;

	for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
		if (JPEGMarkerIsIcc(marker)) {
			if (num_markers == 0)
				num_markers = GETJOCTET(marker->data[13]);
			else if (num_markers != GETJOCTET(marker->data[13]))
				return FALSE;        /* inconsistent num_markers fields */
			seq_no = GETJOCTET(marker->data[12]);
			if (seq_no <= 0 || seq_no > num_markers)
				return FALSE;        /* bogus sequence number */
			if (marker_present[seq_no])
				return FALSE;        /* duplicate sequence numbers */
			marker_present[seq_no] = 1;
			data_length[seq_no] = marker->data_length - JPEG_ICC_OVERHEAD_LEN;
		}
	}

	if (num_markers == 0)
		return FALSE;

	/* Check for missing markers, count total space needed,
	 * compute offset of each marker's part of the data.
	 */

	total_length = 0;
	for (seq_no = 1; seq_no <= num_markers; seq_no++) {
		if (marker_present[seq_no] == 0)
			return FALSE;        /* missing sequence number */
		data_offset[seq_no] = total_length;
		total_length += data_length[seq_no];
	}

	if (total_length <= 0)
		return FALSE;        /* found only empty markers? */

	  /* Allocate space for assembled data */
	icc_data = (JOCTET*)malloc(total_length * sizeof(JOCTET));
	if (icc_data == NULL)
		return FALSE;        /* oops, out of memory */

	  /* and fill it in */
	for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
		if (JPEGMarkerIsIcc(marker)) {
			JOCTET FAR* src_ptr;
			JOCTET* dst_ptr;
			unsigned int length;
			seq_no = GETJOCTET(marker->data[12]);
			dst_ptr = icc_data + data_offset[seq_no];
			src_ptr = marker->data + JPEG_ICC_OVERHEAD_LEN;
			length = data_length[seq_no];
			while (length--) {
				*dst_ptr++ = *src_ptr++;
			}
		}
	}

	*icc_data_ptr = icc_data;
	*icc_data_len = total_length;

	return TRUE;
}


std::shared_ptr<heif_image> loadJPEG(const char* filename)
{
	struct heif_image* image = nullptr;

	// ### Code copied from LibVideoGfx and slightly modified to use HeifPixelImage

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	// to store embedded icc profile
	uint32_t iccLen;
	uint8_t* iccBuffer = NULL;

	// open input file

	FILE* infile;
	if ((infile = fopen(filename, "rb")) == NULL) {
		std::cerr << "Can't open " << filename << "\n";
		exit(1);
	}

	// initialize decompressor

	jpeg_create_decompress(&cinfo);

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_stdio_src(&cinfo, infile);

	/* Adding this part to prepare for icc profile reading. */
	jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xFFFF);

	jpeg_read_header(&cinfo, TRUE);

	boolean embeddedIccFlag = ReadICCProfileFromJPEG(&cinfo, &iccBuffer, &iccLen);

	if (cinfo.jpeg_color_space == JCS_GRAYSCALE) {
		cinfo.out_color_space = JCS_GRAYSCALE;

		jpeg_start_decompress(&cinfo);

		JSAMPARRAY buffer;
		buffer = (*cinfo.mem->alloc_sarray)
			((j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.output_width * cinfo.output_components, 1);

		// create destination image

		struct heif_error err = heif_image_create(cinfo.output_width, cinfo.output_height,
			heif_colorspace_monochrome,
			heif_chroma_monochrome,
			&image);
		(void)err;
		// TODO: handle error

		heif_image_add_plane(image, heif_channel_Y, cinfo.output_width, cinfo.output_height, 8);

		int y_stride;
		uint8_t* py = heif_image_get_plane(image, heif_channel_Y, &y_stride);

		// read the image

		while (cinfo.output_scanline < cinfo.output_height) {
			(void)jpeg_read_scanlines(&cinfo, buffer, 1);

			memcpy(py + (cinfo.output_scanline - 1) * y_stride, *buffer, cinfo.output_width);
		}
	}
	else {
		cinfo.out_color_space = JCS_YCbCr;

		jpeg_start_decompress(&cinfo);

		JSAMPARRAY buffer;
		buffer = (*cinfo.mem->alloc_sarray)
			((j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.output_width * cinfo.output_components, 1);

		// create destination image

		struct heif_error err = heif_image_create(cinfo.output_width, cinfo.output_height,
			heif_colorspace_YCbCr,
			heif_chroma_420,
			&image);
		(void)err;

		heif_image_add_plane(image, heif_channel_Y, cinfo.output_width, cinfo.output_height, 8);
		heif_image_add_plane(image, heif_channel_Cb, (cinfo.output_width + 1) / 2, (cinfo.output_height + 1) / 2, 8);
		heif_image_add_plane(image, heif_channel_Cr, (cinfo.output_width + 1) / 2, (cinfo.output_height + 1) / 2, 8);

		int y_stride;
		int cb_stride;
		int cr_stride;
		uint8_t* py = heif_image_get_plane(image, heif_channel_Y, &y_stride);
		uint8_t* pcb = heif_image_get_plane(image, heif_channel_Cb, &cb_stride);
		uint8_t* pcr = heif_image_get_plane(image, heif_channel_Cr, &cr_stride);

		// read the image

		//printf("jpeg size: %d %d\n",cinfo.output_width, cinfo.output_height);

		while (cinfo.output_scanline < cinfo.output_height) {
			JOCTET* bufp;

			(void)jpeg_read_scanlines(&cinfo, buffer, 1);

			bufp = buffer[0];

			int y = cinfo.output_scanline - 1;

			for (unsigned int x = 0; x < cinfo.output_width; x += 2) {
				py[y * y_stride + x] = *bufp++;
				pcb[y / 2 * cb_stride + x / 2] = *bufp++;
				pcr[y / 2 * cr_stride + x / 2] = *bufp++;

				if (x + 1 < cinfo.output_width) {
					py[y * y_stride + x + 1] = *bufp++;
				}

				bufp += 2;
			}

			if (cinfo.output_scanline < cinfo.output_height) {
				(void)jpeg_read_scanlines(&cinfo, buffer, 1);

				bufp = buffer[0];

				y = cinfo.output_scanline - 1;

				for (unsigned int x = 0; x < cinfo.output_width; x++) {
					py[y * y_stride + x] = *bufp++;
					bufp += 2;
				}
			}
		}
	}

	if (embeddedIccFlag && iccLen > 0) {
		heif_image_set_raw_color_profile(image, "prof", iccBuffer, (size_t)iccLen);
	}

	// cleanup
	free(iccBuffer);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	fclose(infile);

	return std::shared_ptr<heif_image>(image,
		[](heif_image* img) { heif_image_release(img); });
}