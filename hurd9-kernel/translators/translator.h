// translator.h - Framework for Hurd translators on 9P
// Extract Hurd's translator model, run on 9P instead of Mach

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

// Universal translator interface (extracted from Hurd's trivfs)
typedef struct Translator9P {
    char *name;
    void *private_data;
    
    // Core operations (map directly from Hurd)
    void (*open)(Req *r);
    void (*read)(Req *r);  
    void (*write)(Req *r);
    void (*stat)(Req *r);
    void (*create)(Req *r);
    void (*remove)(Req *r);
    void (*wstat)(Req *r);
    void (*destroyfid)(Fid *f);
    
    // Translator-specific hooks
    int (*init)(struct Translator9P *t, char **argv);
    void (*cleanup)(struct Translator9P *t);
    
    // Stacking support (Hurd's killer feature)
    struct Translator9P *below;  // Translator we're stacked on
    struct Translator9P *above;  // Translator stacked on us
    
    // Filter/transform capability  
    void* (*filter_read)(void *data, size_t *len);
    void* (*filter_write)(void *data, size_t *len);
    
    // 9P.e enhanced operations
    void (*async_read)(Req *r);
    void (*async_write)(Req *r); 
    void (*stream_in)(Req *r);
    void (*stream_out)(Req *r);
    void (*notify)(Req *r);
    
} Translator9P;

// Standard translator file structure
typedef struct TranslatorFile {
    char *name;
    int mode;       // File permissions
    int qtype;      // QTFILE, QTDIR, etc
    void *data;     // File-specific data
    off_t offset;   // Current offset
} TranslatorFile;

// Directory structure for complex translators
typedef struct TranslatorDir {
    char *name;
    TranslatorFile *files;
    int nfiles;
    struct TranslatorDir *subdirs;
    int nsubdirs;
} TranslatorDir;

// Generic operations that work for most translators
void translator_stat(Req *r);
void translator_open(Req *r);
void translator_walk(Req *r);
void translator_clunk(Req *r);

// Stack translator on another
int translator_stack(Translator9P *above, Translator9P *below);

// Create and serve a translator
void serve_translator(Translator9P *t, char *srvname, char *mtpt);

// Template for simple translators (like hello, null, zero)
typedef struct SimpleTranslator {
    Translator9P base;
    char *content;
    size_t content_len;
    int readable;
    int writable;
} SimpleTranslator;

SimpleTranslator* simple_translator_new(char *name, char *content, int readable, int writable);

// Template for device translators (our universal device framework)
typedef struct DeviceTranslator {
    Translator9P base;
    
    // Standard device files
    char *ctl_content;      // /dev/device/ctl
    char *status_content;   // /dev/device/status  
    char *info_content;     // /dev/device/info
    
    // Device operations
    int (*device_ctl)(char *cmd, char **response);
    int (*device_read)(void *buf, size_t size, off_t offset);
    int (*device_write)(void *buf, size_t size, off_t offset);
    
    // Enhanced operations
    int (*async_op)(void *op_data, u64int *job_id);
    int (*stream_setup)(int direction, u32int chunk_size);
    
} DeviceTranslator;

DeviceTranslator* device_translator_new(char *name);

// Template for filesystem translators (like ext2fs, fatfs)
typedef struct FSTranslator {
    Translator9P base;
    
    // Filesystem operations
    int (*fs_lookup)(char *path, TranslatorFile **file);
    int (*fs_create)(char *path, int mode);
    int (*fs_remove)(char *path);
    int (*fs_rename)(char *from, char *to);
    
    // Block device interface
    int (*read_blocks)(u64int block, u32int count, void *buf);
    int (*write_blocks)(u64int block, u32int count, void *buf);
    
} FSTranslator;

FSTranslator* fs_translator_new(char *name, char *device);

// Utility functions extracted from Hurd
error_t check_open_permissions(int requested_mode, int allowed_mode);
void update_file_times(TranslatorFile *f);
int parse_translator_args(int argc, char **argv, char **srvname, char **mtpt);

// 9P.e enhanced message handling
void handle_9pe_async(Req *r, Translator9P *t);
void handle_9pe_stream(Req *r, Translator9P *t);  
void handle_9pe_batch(Req *r, Translator9P *t);
void handle_9pe_notify(Req *r, Translator9P *t);

// Error conversion (Hurd errno → 9P error strings)
char* hurd_error_to_9p(error_t err);

#define TRANSLATOR_MAIN(t) \
    void threadmain(int argc, char **argv) { \
        char *srvname = nil, *mtpt = nil; \
        if(parse_translator_args(argc, argv, &srvname, &mtpt) < 0) \
            exits("usage"); \
        if((t)->init && (t)->init((t), argv) < 0) \
            exits("init failed"); \
        serve_translator((t), srvname, mtpt); \
    }