#include "JpgToHeic.h"
#include "LoadJpeg.h"
void JpgToHeic(const std::string& input_filename, const std::string& output_filename, int quality)
{
	std::shared_ptr<heif_image> image;

	image = loadJPEG(input_filename.c_str());

	heif_context* ctx = heif_context_alloc();

	// get the default encoder
	heif_encoder* encoder;
	heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);

	// set the encoder parameters
	heif_encoder_set_lossy_quality(encoder, quality);

	// encode the image
	heif_context_encode_image(ctx, image.get(), encoder, nullptr, nullptr);

	heif_encoder_release(encoder);

	heif_context_write_to_file(ctx, output_filename.c_str());
}