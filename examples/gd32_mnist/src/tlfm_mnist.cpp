#include "tlfm_mnist.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "img_capt.h"
#ifdef __cplusplus
}
#endif

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "model.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <stdio.h>

#define SIMAGE_WIDTH 20
#define SIMAGE_HEIGHT 20

//#define MIN_WIDTH (SIMAGE_WIDTH / 2)
#define MIN_WIDTH (SIMAGE_WIDTH / 2)
#define MIN_HEIGHT (SIMAGE_HEIGHT / 2)

#define TENSOR_ARENA_SIZE	(3*1024)
uint8_t tlfm_mnist_tensor_arena[TENSOR_ARENA_SIZE];
tflite::MicroInterpreter *tlfm_mnist_interpreter = nullptr;

#define INPUT_ELEM	(SIMAGE_WIDTH * SIMAGE_HEIGHT)
#define INPUT_SIZE	(INPUT_ELEM * sizeof(float))
float *tlfm_mnist_input = nullptr;

#define OUTPUT_ELEM	(10)
#define OUTPUT_SIZE	(INPUT_ELEM * sizeof(float))
float *tlfm_mnist_output = nullptr;

int tlfm_mnist_last_result = -1;

static int prepare_model()
{

	// Map the model into a usable data structure. This doesn't involve any
	// copying or parsing, it's a very lightweight operation.
	const tflite::Model *model = ::tflite::GetModel(g_model);
	if (model->version() != TFLITE_SCHEMA_VERSION) {
		printf(
			"Model provided is schema version %li not equal "
			"to supported version %i.\n",
			model->version(), TFLITE_SCHEMA_VERSION);
		return kTfLiteError;
	}

	tflite::MicroMutableOpResolver<5> *resolver =
		new tflite::MicroMutableOpResolver<5>;
	if (resolver == nullptr) {
		printf("Resolver alloc failed.");
		return kTfLiteError;
	}
	if (resolver->AddFullyConnected() != kTfLiteOk) {
		return kTfLiteError;
	}
	if (resolver->AddReshape() != kTfLiteOk) {
		return kTfLiteError;
	}
	// nur für quantisierung
	if (resolver->AddQuantize() != kTfLiteOk) {
		return kTfLiteError;
	}
	if (resolver->AddDequantize() != kTfLiteOk) {
		return kTfLiteError;
	}
	if (resolver->AddSoftmax() != kTfLiteOk) {
		return kTfLiteError;
	}
#if 0
	if (resolver->AddConv2D() != kTfLiteOk) {
		return kTfLiteError;
	}
	if (resolver->AddMaxPool2D() != kTfLiteOk) {
		return kTfLiteError;
	}
#endif

	// Build an interpreter to run the model with
	tlfm_mnist_interpreter = new tflite::MicroInterpreter(model, *resolver,
			tlfm_mnist_tensor_arena, sizeof(tlfm_mnist_tensor_arena));
	if (tlfm_mnist_interpreter == nullptr) {
		printf("Interpretor alloc failed.");
		return kTfLiteError;
	}

	// Allocate memory from the tensor_arena for the model's tensors
	if (tlfm_mnist_interpreter->AllocateTensors() != kTfLiteOk) {
		printf("Allocate tensor failed.");
		return kTfLiteError;
	}

	// Obtain a pointer to the model's input tensor
	TfLiteTensor* input = tlfm_mnist_interpreter->input(0);
	if (input == nullptr) {
		printf("Input tensor in null.");
		return kTfLiteError;
	}
	tlfm_mnist_input = input->data.f;

	// Obtain a pointer to the output tensor.
	TfLiteTensor* output = tlfm_mnist_interpreter->output(0);
	if (output == nullptr) {
		printf("Output tensor in null.");
		return kTfLiteError;
	}
	tlfm_mnist_output = output->data.f;

	return kTfLiteOk;
}


int infer()
{
	TfLiteStatus invoke_status = tlfm_mnist_interpreter->Invoke();
	if (invoke_status != kTfLiteOk) {
		printf("Interpreter invocation failed.");
		return -2;
	}
	float max_value = -100.0;
	int max_index = -1;
	for (int i = 0; i < OUTPUT_ELEM; i++) {
		printf("%i = %i \n", i, (int)(100.0 * tlfm_mnist_output[i]));
		if (tlfm_mnist_output[i] > max_value) {
			max_value = tlfm_mnist_output[i];
			max_index = i;
		}
	}
	printf("%s.%i max_index = %i\n", __FUNCTION__, __LINE__, max_index);

	return max_index;
}

#define DUMP_MAX_WIDTH	256
static void dump_input(float *image, unsigned int width, unsigned int height)
{
	char hline[DUMP_MAX_WIDTH + 1];
	char line[DUMP_MAX_WIDTH + 1];
	unsigned int x, y, i;

	if (width + 4 > DUMP_MAX_WIDTH) {
		printf("%s: width %i to big\n", __FUNCTION__, width);
		return;
	}
	for (x = 0; x < width + 4; x++) {
		hline[x] = '-';
	}
	hline[x] = 0;

	printf("%s\n", hline);
	for (y = 0; y < height; y++) {
		i = 0;
		line[i++] = '|';
		line[i++] = ' ';
		for (x = 0; x < width; x++) {
			float val = image[y * height + x];
			char c;
			if (val > 0.9) {
				c = '#';
			} else if (val > 0.4) {
				c = '*';
			} else if (val > 0.0) {
				c = '.';
			} else {
				c = ' ';
			}
			line[i++] = c;
		}
		line[i++] = ' ';
		line[i++] = '|';
		line[i] = 0;
		printf("%s\n", line);
	}
	printf("%s\n", hline);

#if 0
	/* works only with fp support */
	printf("val = np.array([");
	for (y = 0; y < height; y++) {
		//printf(",[");
		for (x = 0; x < width; x++) {
			printf("%f, ", image[y * height + x]);
		}
	}
	printf("], dtype=float).reshape(1, 28, 28, 1)\n");
#endif
}

#if 0
#define FILTER_AMP	1.5
const float filter[3][3] = {
	{0.1, 0.2, 0.1},
	{0.2, 0.3, 0.2},
	{0.1, 0.2, 0.1},
};
#else
#define FILTER_AMP	(1.0/16.0) * 1.8
const float filter[3][3] = {
	{1, 2, 1},
	{2, 4, 2},
	{1, 2, 1},
};
#endif


static void preprocess_input(struct bimg *bimg)
{
	int x, y, xs, ys;
	int width = bimg->x_max - bimg->x_min + 1;
	int height = bimg->y_max - bimg->y_min + 2;

	/* reset */
	memset(tlfm_mnist_input, 0, INPUT_SIZE);

	if (width < MIN_WIDTH) {
		width = MIN_WIDTH;
	}
	if (width < MIN_HEIGHT) {
		width = MIN_HEIGHT;
	}

	printf("w=%i h=%i\n", width, height);

	for (ys = 0 ; ys < SIMAGE_HEIGHT; ys++) {
		y = bimg->y_min + (ys * height) / SIMAGE_HEIGHT + 1;
		for (xs = 0 ; xs < SIMAGE_WIDTH; xs++) {
			x = bimg->x_min + (xs * width) / SIMAGE_WIDTH + 1;

			float val = 0.0;
			for (int i = -1; i < 1; i++) {
				for (int j = -1; j < 1; j++) {
					if (bimg_get(bimg, x + j, y + i)) {
						val += filter[i + 1][j + 1] * FILTER_AMP;
						/* clamp */
						if (val > 1.0) {
							val = 1.0;
						}
					}
				}
			}

			tlfm_mnist_input[ys * SIMAGE_HEIGHT + xs] = val;
		}
	}
}

int tlfm_get_result()
{
	return tlfm_mnist_last_result;
}

int tlfm_mnist_infer(struct bimg *bimg)
{
	if (bimg == NULL) {
		return -1;
	}

	printf("Infer:\n");
	bimg_dump(bimg);
	preprocess_input(bimg);
	dump_input(tlfm_mnist_input, SIMAGE_WIDTH, SIMAGE_HEIGHT);
	return infer();
}

int tlfm_mnist_init()
{
	prepare_model();
	tlfm_mnist_last_result = -1;
	return 0;
}
