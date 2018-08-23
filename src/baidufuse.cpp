#include "baidufuse.h"
#include "cache.h"
#include "utils.h"

#include <errno.h>


void baidu_prepare(){
    cache_prepare();
}

void *baidu_init(struct fuse_conn_info *conn){
    conn->capable = conn->want & FUSE_CAP_BIG_WRITES & FUSE_CAP_EXPORT_SUPPORT;
    conn->max_background = 20;
    conn->max_write = 1024*1024;
    conn->max_readahead = 10*1024*1024;
    return cache_root();
}

void baidu_destroy(void* root){
    delete (entry_t*)root;
}

int baidu_statfs(const char *path, struct statvfs *sf){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    return root->statfs(path, sf);
}

int baidu_opendir(const char *path, struct fuse_file_info *fi){
    return baidu_open(path, fi);
}

int baidu_readdir(const char*, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    entry_t* entry = (entry_t*)fi->fh;
    auto entrys = entry->entrys();
    for(auto i: entrys){
        struct stat st;
        i.second->getattr(&st);
        filler(buf, i.first.c_str(), &st, 0);
    }
    return 0;
}

int baidu_releasedir(const char*, struct fuse_file_info *fi){
    return baidu_release(nullptr, fi);
}

int baidu_getattr(const char *path, struct stat *st){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    entry_t* entry = root->find(path);
    if(entry == nullptr){
        return -ENOENT;
    }
    entry->getattr(st);
    return 0;
}


int baidu_mkdir(const char *path, mode_t mode){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    entry_t* entry =  root->find(dirname(path));
    if(entry == nullptr){
        return -ENOENT;
    }
    if(entry->mkdir(basename(path)) == nullptr){
        return -errno;
    }
    return 0;
}

int baidu_unlink(const char *path){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    entry_t *entry = root->find(path);
    if(entry == nullptr){
        return -ENOENT;
    }
    return entry->unlink();
}

int baidu_rmdir(const char *path){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    entry_t *entry = root->find(path);
    if(entry == nullptr){
        return -ENOENT;
    }
    return entry->rmdir();
}

int baidu_rename(const char *oldname, const char *newname){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    entry_t* entry =  root->find(oldname);
    if(entry == nullptr){
        return -ENOENT;
    }
    entry_t* nentry = root->find(newname);
    if(nentry){
        nentry->unlink();
    }
    entry_t* newparent = root->find(dirname(newname));
    if(newparent == nullptr){
        return -ENOENT;
    }
    return entry->move(newparent, basename(newname));
}

int baidu_create(const char *path, mode_t mode, struct fuse_file_info *fi){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    entry_t *entry = root->find(dirname(path));
    assert(entry);
    entry_t *nentry = entry->create(basename(path));
    if(nentry == nullptr){
        return -errno;
    }
    fi->fh = (uint64_t)nentry;
    fi->direct_io = 1;
    return nentry->open();
}

int baidu_open(const char *path, struct fuse_file_info *fi){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    entry_t* entry = root->find(path);
    if(entry == nullptr){
        return -ENOENT;
    }
    fi->fh = (uint64_t)entry;
    fi->direct_io = 1;
    return entry->open();
}

int baidu_truncate(const char* path, off_t offset){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    entry_t* entry = root->find(path);
    if(entry == nullptr){
        return -ENOENT;
    }
    return entry->truncate(offset);
}

int baidu_fgetattr(const char*, struct stat* st, struct fuse_file_info* fi){
    entry_t* entry = (entry_t*)fi->fh;
    return entry->getattr(st);
}

int baidu_read(const char *, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    entry_t* entry = (entry_t*)fi->fh;
    return entry->read(buf, offset, size);
}

int baidu_ftruncate(const char*, off_t offset, struct fuse_file_info *fi){
    entry_t* entry = (entry_t*)fi->fh;
    return entry->truncate(offset);
}

int baidu_write(const char *, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    entry_t* entry = (entry_t*)fi->fh;
    return entry->write(buf, offset, size);
}

int baidu_fsync(const char *, int datasync, struct fuse_file_info *fi){
    entry_t* entry = (entry_t*)fi->fh;
    return entry->sync(datasync);
}

int baidu_flush(const char*, struct fuse_file_info *fi){
    entry_t* entry = (entry_t*)fi->fh;
    return entry->flush();
}

int baidu_release(const char *, struct fuse_file_info *fi){
    entry_t* entry = (entry_t*)fi->fh;
    return entry->release();
}

int baidu_utimens(const char *path, const struct timespec tv[2]){
    entry_t* root = (entry_t*)fuse_get_context()->private_data;
    entry_t* entry = (entry_t*)root->find(path);
    if(entry == nullptr){
        return -ENOENT;
    }
    return entry->utime(tv);
}

int baidu_setxattr(const char *path, const char *name, const char *value, size_t size, int flags){
    return -ENOTSUP;
}

int baidu_getxattr(const char *path, const char *name, char *value, size_t len){
    return -ENOTSUP;
}
