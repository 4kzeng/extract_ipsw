#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


#include <zip.h>

#ifdef WIN32
#define strcasecmp _stricmp
#endif

int stringendswith(const char *s, const char *t)
{
    size_t slen = strlen(s);
    size_t tlen = strlen(t);
    if (tlen > slen) return 1;
    return strcmp(s + slen - tlen, t);
}

void ensurefilepath(const char* filepath)
{

}

void plist_read_from_buffer(plist_t *pplist, const char* buffer, uint64_t length)
{
    if (buffer == NULL || length <= 8) return;

    if (memcmp(buffer, "bplist00", 8) == 0)
    {
        plist_from_bin(buffer, length, pplist);
    }
    else
    {
        plist_from_xml(buffer, length, pplist);
    }
}


int extract_memory(const char *ipsw, const char* internal_path, unsigned char **buf, uint64_t *len)
{
    size_t size = 0;
    unsigned char* buffer = NULL;

    int err = 0;
    struct zip *hzip = zip_open(ipsw, 0, &err);
    if (hzip == NULL) {
        return -1;
    }

    int zindex = zip_name_locate(hzip, internal_path, 0);
    if (zindex < 0) {
        zip_close(hzip);
        return -1;
    }

    struct zip_stat zstat;
    zip_stat_init(&zstat);
    if (zip_stat_index(hzip, zindex, 0, &zstat) != 0) {
        zip_close(hzip);
        return -1;
    }

    struct zip_file* zfile = zip_fopen_index(hzip, zindex, 0);
    if (zfile == NULL) {
        zip_close(hzip);
        return -1;
    }

    size = zstat.size;
    buffer = (unsigned char*) malloc(size+1);
    if (buffer == NULL) {
        zip_fclose(zfile);
        zip_close(hzip);
        return -1;
    }

    if (zip_fread(zfile, buffer, size) != size) {
        zip_fclose(zfile);
        free(buffer);
        zip_close(hzip);
        return -1;
    }

    buffer[size] = '\0';

    zip_fclose(zfile);

    zip_close(hzip);
    *buf = buffer;
    *len = size;

    return 0;
}



int extract_file(const char* ipsw, const char* internal_path, const char* out_path)
{
    const size_t buffersize = 0;
    unsigned char* buffer[buffersize];
    FILE* fdOut = NULL;



    int err = 0;
    struct zip *hzip = zip_open(ipsw, 0, &err);
    if (hzip == NULL) {
        return -1;
    }

    int zindex = zip_name_locate(hzip, internal_path, 0);
    if (zindex < 0) {
        zip_close(hzip);
        return -1;
    }

    struct zip_stat zstat;
    zip_stat_init(&zstat);
    if (zip_stat_index(hzip, zindex, 0, &zstat) != 0) {
        zip_close(hzip);
        return -1;
    }

    struct zip_file* zfile = zip_fopen_index(hzip, zindex, 0);
    if (zfile == NULL) {
        zip_close(hzip);
        return -1;
    }

    fdOut = fopen(out_path, "wb+");
    if (fdOut == NULL) {
        zip_fclose(zfile);
        zip_close(hzip);
        return -1;
    }

    while(true){
        uint64_t readBytes =  zip_fread(zfile, buffer, buffersize);
        if( readBytes == 0 ){
            break;
        }
        else if( readBytes < 0 ){
            zip_fclose(zfile);
            zip_close(hzip);
            fclose(fdOut);
            remove(out_path);
            return -1;
        }
        else{
            fwrite(buffer, 1, readBytes, fdOut);
        }
    }


    zip_fclose(zfile);
    zip_close(hzip);
    fclose(fdOut);

    return 0;
}

plist_t extract_buildmanifest_plist(const char *ipsw)
{
    plist_t manifest = NULL;
    unsigned char *pbmbuf = NULL;
    uint64_t pbmsize = 0;
    if( extract_memory(ipsw, "BuildManifest.plist", &pbmbuf, &pbmsize) < 0 ){
        return NULL;
    }

    plist_read_from_buffer(&manifest, (const char*)pbmbuf, pbmsize);
    free(pbmbuf);
    return manifest;
}

plist_t find_buildid(plist_t buildmanifest, const char *hardware_model, bool isErase)
{
    const char* variant = isErase? "Erase" : "Upgrade";

    plist_t build_identities_array = plist_dict_get_item(buildmanifest, "BuildIdentities");
    if (!build_identities_array || plist_get_node_type(build_identities_array) != PLIST_ARRAY) {
        return NULL;
    }

    uint32_t i=0;
    for (i = 0; i < plist_array_get_size(build_identities_array); i++) {
        plist_t ident = plist_array_get_item(build_identities_array, i);
        if (!ident || plist_get_node_type(ident) != PLIST_DICT) {
            continue;
        }
        plist_t info_dict = plist_dict_get_item(ident, "Info");
        if (!info_dict || plist_get_node_type(ident) != PLIST_DICT) {
            continue;
        }
        plist_t devclass = plist_dict_get_item(info_dict, "DeviceClass");
        if (!devclass || plist_get_node_type(devclass) != PLIST_STRING) {
            continue;
        }
        char* str = NULL;
        plist_get_string_val(devclass, &str);
        if (strcasecmp(str, hardware_model) != 0) {
            free(str);
            continue;
        }
        free(str);
        str = NULL;
        if (variant) {
            plist_t rvariant = plist_dict_get_item(info_dict, "Variant");
            if (!rvariant || plist_get_node_type(rvariant) != PLIST_STRING) {
                continue;
            }
            str = (char*)plist_get_string_ptr(rvariant, NULL);
            if ( strstr(str, variant) && !strstr(str, "Research")) {
                return ident;
            }
        }
    }

    return NULL;
}

int find_component_path(plist_t build_id, const char *comp, const char **internal_path)
{
    char* filename = NULL;

    plist_t manifest_node = plist_dict_get_item(build_id, "Manifest");
    if (!manifest_node || plist_get_node_type(manifest_node) != PLIST_DICT) {
        if (filename)
            free(filename);
        return -1;
    }

    plist_t component_node = plist_dict_get_item(manifest_node, comp);
    if (!component_node || plist_get_node_type(component_node) != PLIST_DICT) {
        if (filename)
            free(filename);
        return -1;
    }

    plist_t component_info_node = plist_dict_get_item(component_node, "Info");
    if (!component_info_node || plist_get_node_type(component_info_node) != PLIST_DICT) {
        if (filename)
            free(filename);
        return -1;
    }

    plist_t component_info_path_node = plist_dict_get_item(component_info_node, "Path");
    if (!component_info_path_node || plist_get_node_type(component_info_path_node) != PLIST_STRING) {
        if (filename)
            free(filename);
        return -1;
    }
    plist_get_string_val(component_info_path_node, &filename);

    *internal_path = filename;
    return 0;
}
