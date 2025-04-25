#include "dr_api.h"
#include "dr_string.h"


#include <unistd.h> //getpid();
#include <string.h> // strtok...
#include <stdint.h>
#include "sys/syscall.h"

const int TYPE_HW = 0;
const int TYPE_SW = 1;

typedef struct _collection_t {
    uint64_t time_stamp;
    uint32_t type;
    uint32_t pc;
} collection_t;

// Max number of traced collections a buffer can have
#define MAX_NUM_COLLECTIONS_REFS 8192
/* The size of the memory buffer for holding collections. When it fills up,
  we dump data from the buffer to the file*/
#define FILE_BUF_SIZE (sizeof(collection_t) * MAX_NUM_COLLECTIONS_REFS )

/* buffer */
typedef struct _buffer_t{
    char *buf_ptr;
    char *buf_base;
    /* buf_end holds the negative value of real address of buffer end. */
    ptr_int_t buf_end;
} buffer_t;

static file_t trace_file;
static buffer_t *buffer;

#define BUFFER_SIZE_BYTES(buf) sizeof(buf)
#define BUFFER_SIZE_ELEMENTS(buf) (BUFFER_SIZE_BYTES(buf) / sizeof((buf)[0]))
#define BUFFER_LAST_ELEMENT(buf) (buf)[BUFFER_SIZE_ELEMENTS(buf) - 1]
#define NULL_TERMINATE_BUFFER(buf) BUFFER_LAST_ELEMENT(buf) = 0

#define MAX_ADDRESS_LINES 512000
#define MAPS_LINE_LENGTH 8192
#define MAPS_LINE_FORMAT4 "%08lx-%08lx %s %*x %*s %*u %4096s"
#define MAPS_LINE_MAX4 49 /* sum of 8  1  8  1 4 1 8 1 5 1 10 1 */
#define MAPS_LINE_FORMAT8 "%016lx-%016lx %s %*x %*s %*u %4096s"
#define MAPS_LINE_MAX8 73 /* sum of 16  1  16  1 4 1 16 1 5 1 10 1 */
#define MAPS_LINE_MAX MAPS_LINE_MAX8
#define MAX_VP_RIPS 0xFFFFF
#define MAX_SW_PCS 0xFFFFFFF

static uint64 base_address;

static void parse_trace_addresses();

static uint trace_pc = 0;

static bool vp_address[MAX_VP_RIPS] = {false};
static bool sw_address[MAX_SW_PCS] = {false};

static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating);
static void event_exit(void);

static void flush_file();

static void clean_call_add_rip(uint rip);

static void clean_call_add_pc(uint reg);

static uint64_t time_diff(struct timeval *start, struct timeval *end);

void dr_memset(void *buf, int value, size_t count) {
    unsigned char *ptr = (unsigned char *)buf;
    while (count--) {
        *ptr++ = (unsigned char)value;
    }
}

bool endsWithApplicationName(char * full_path){
    const char * appName = dr_get_application_name();
    for(int i = 0; full_path[i] != '\0'; ++i){
        bool foundAll = true;
        for(int j = 0; appName[j] != '\0'; ++j){
            if(full_path[i+j] != appName[j]){
                foundAll = false;
            }
        }
        if(foundAll){
            return true;
        }
    }
    return false; 
}

static void event_module_load(void *drcontext, const module_data_t *info, bool loaded) {
    if(endsWithApplicationName(info->full_path)){
        base_address = (uint64)info->start;
    }
    
}
uint64 getTimeInMilliseconds(){
    // dr_time_micro_t time;
    // dr_get_time_microseconds(&time);
    // return ((((time.hour*60 + time.minute)*60 + time.second)*1000 + time.milliseconds)*1000 + time.microseconds);
    dr_time_t time;
    dr_get_time(&time);
    return (((time.hour*60 + time.minute)*60 + time.second)*1000 + time.milliseconds);
    // struct timeval tv;
    // dynamorio_syscall(96, 2, &tv, NULL);
    // return tv.tv_sec * 1000000LL + tv.tv_usec;


}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
    if(argc < 2) {
        dr_printf("Trace file missing!\n");
        return;
    }

    parse_trace_addresses();

    dr_register_module_load_event(event_module_load);
    dr_register_exit_event(event_exit);
    dr_register_bb_event(event_basic_block);

    
    buffer = dr_global_alloc(sizeof(buffer));
    buffer->buf_base = dr_global_alloc(FILE_BUF_SIZE);
    buffer->buf_ptr = buffer->buf_base;
    buffer->buf_end = (ptr_int_t)(buffer->buf_base + FILE_BUF_SIZE);

    dr_delete_file(argv[1]);

    trace_file = dr_open_file(argv[1], DR_FILE_WRITE_OVERWRITE);
    ((collection_t *)(buffer->buf_ptr))->type = 0;
    ((collection_t *)(buffer->buf_ptr))->time_stamp = 0;
    ((collection_t *)(buffer->buf_ptr))->pc = 0;

}

int convertToInteger(char* string, int length){
    int result = 0;
    int decimal = 1;
    for(int i = length-1; i >= 0; --i){
        int converted = -1;
        if(string[i] == '0'){
            converted = 0;
        }else if(string[i] == '1'){
            converted = 1;
        }else if(string[i] == '2'){
            converted = 2;
        }else if(string[i] == '3'){
            converted = 3;
        }else if(string[i] == '4'){
            converted = 4;
        }else if(string[i] == '5'){
            converted = 5;
        }else if(string[i] == '6'){
            converted = 6;
        }else if(string[i] == '7'){
            converted = 7;
        }else if(string[i] == '8'){
            converted = 8;
        }else if(string[i] == '9'){
            converted = 9;
        }
        result += decimal *  converted;
        decimal *= 10;
    }
    return result;
}

static void parse_trace_addresses() {
    char line[MAX_ADDRESS_LINES];
    file_t trace_addresses = dr_open_file("<FLD_FOLDER_PATH>/files/address.tbl", DR_FILE_READ);
    if(trace_addresses == INVALID_FILE){
        dr_printf("No such file\n");
        return;
    }
    uint64 read = dr_read_file(trace_addresses, &line, MAX_ADDRESS_LINES);
    dr_printf("READ: %d\n", read);
    char token[256] = "";
    int token_index = 0;
    int token_id = -1;
    for(int i = 0; i < read; ++i){
        if(line[i] == '\n'){
            token[token_index] = '\0';
            int address = convertToInteger(token, token_index);
            if(token_id == 0){
                trace_pc = address;
            }else if(token_id == 1){
                vp_address[address] = true;
            }else if(token_id == 2){
                sw_address[address] = true;
            }
            //dr_printf("%d->%d\n", token_id, address);
            token_index = 0;
            token[0] = '\0';
        }else{
            if(line[i] == ':'){
                token[token_index] = '\0';
                if(token[0] == '1'){
                    token_id = 1;
                }else if(token[0] == '2'){
                    token_id = 2;
                }else if(token[0] == '0'){
                    token_id = 0;
                }else{
                    //dr_printf("%s\n", token);
                }
                token_index = 0;
                token[0] = '\0';
            }else{
                token[token_index] = line[i];
                token_index++;
            }
        }
    }
}

static dr_emit_flags_t event_basic_block(void *drcontext, void *tag, instrlist_t *bb, bool for_trace, bool translating) {
    for(instr_t *instr = instrlist_first(bb); instr != NULL; instr = instr_get_next(instr)) {
        /* TRACE RIPS*/
        uint64 pc = (uint64)(instr_get_app_pc(instr));
        //dr_printf("PC_B_4: %llx\n", pc);
        pc = pc - base_address;
        uint pc_32 = pc & 0xFFFFFFFF;
        //dr_printf("PC_32: %x \nPC: %llx\n", pc_32, pc);
        if(pc_32 > 0 && pc_32 < MAX_VP_RIPS && vp_address[pc_32]) {
            dr_insert_clean_call(drcontext, bb, instr, clean_call_add_rip, false, 1, OPND_CREATE_INT32(pc_32));
        }
        /* END TRACE RIPS */

        /* TRACE PC */
        if(pc_32 == trace_pc){
            if(instr_writes_memory(instr)) {
                dr_insert_clean_call(drcontext, bb, instr, clean_call_add_pc, false, 1, OPND_CREATE_INT32(opnd_get_reg(instr_get_src(instr, 0))));
            }
        }
        /* END TRACE PC*/
    }

    return DR_EMIT_DEFAULT;
}

static void event_exit(void) {
    flush_file();
    dr_close_file(trace_file);
    dr_global_free(buffer->buf_base, FILE_BUF_SIZE);
    dr_global_free(buffer, sizeof(buffer_t));
    dr_unregister_bb_event(event_basic_block);
    dr_unregister_exit_event(event_exit);
}
  
static void flush_file() {
    collection_t *collection_ref;

    collection_ref = (collection_t *)buffer->buf_base;
    dr_write_file(trace_file, buffer->buf_base, (size_t)(buffer->buf_ptr - buffer->buf_base));
    dr_memset(buffer->buf_base, 0, FILE_BUF_SIZE);
    buffer->buf_ptr = buffer->buf_base;
}

static void clean_call_add_rip(uint rip) {
    ((collection_t *)(buffer->buf_ptr))->type = TYPE_HW;
    ((collection_t *)(buffer->buf_ptr))->time_stamp = getTimeInMilliseconds();
    ((collection_t *)(buffer->buf_ptr))->pc = rip;

    buffer->buf_ptr += sizeof(collection_t);
    if ((ptr_int_t)(buffer->buf_ptr) >= buffer->buf_end) {
        flush_file();
    }
    
    ((collection_t *)(buffer->buf_ptr))->type = 0;
    ((collection_t *)(buffer->buf_ptr))->time_stamp = 0;
    ((collection_t *)(buffer->buf_ptr))->pc = 0;
}

static void clean_call_add_pc(uint reg){
    dr_mcontext_t mcontext = {
        sizeof(mcontext),
        DR_MC_ALL,
    };
    void * drcontext = dr_get_current_drcontext();
    dr_get_mcontext(drcontext, &mcontext);
    reg_t pc = reg_get_value(reg, &mcontext);

    if(pc < MAX_SW_PCS && sw_address[pc]){
        ((collection_t *)(buffer->buf_ptr))->type = TYPE_SW;
        ((collection_t *)(buffer->buf_ptr))->time_stamp = getTimeInMilliseconds();
        ((collection_t *)(buffer->buf_ptr))->pc = pc;

        buffer->buf_ptr += sizeof(collection_t);
        if ((ptr_int_t)(buffer->buf_ptr) >= buffer->buf_end) {
            flush_file();
        }
        
        ((collection_t *)(buffer->buf_ptr))->type = 0;
        ((collection_t *)(buffer->buf_ptr))->time_stamp = 0;
        ((collection_t *)(buffer->buf_ptr))->pc = 0;
    }
}
