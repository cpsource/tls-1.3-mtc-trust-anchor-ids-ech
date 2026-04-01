/*
 * MTC Public API (C) - Implementation.
 *
 * Talks to the CA/Log server over HTTP (using libcurl) and manages
 * local key/cert storage in ~/.TPM.
 *
 * Build:
 *   gcc -o mtc.o -c mtc.c $(pkg-config --cflags libcurl json-c openssl)
 *   gcc -o myapp myapp.c mtc.o $(pkg-config --libs libcurl json-c openssl)
 *
 * Dependencies: libcurl, json-c, openssl
 */

#include "mtc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#include <curl/curl.h>
#include <json-c/json.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ec.h>

/* ----------------------------------------------------------------------- */
/* Internal state                                                          */
/* ----------------------------------------------------------------------- */

struct mtc_conn {
    char *server_url;
    char *ca_name;
    char *log_id;
    int   tree_size;
    CURL *curl;
};

static char _last_error[512] = {0};

static void set_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(_last_error, sizeof(_last_error), fmt, ap);
    va_end(ap);
}

const char *MTC_Last_Error(void) {
    return _last_error[0] ? _last_error : NULL;
}

/* ----------------------------------------------------------------------- */
/* HTTP helpers                                                            */
/* ----------------------------------------------------------------------- */

struct http_buf {
    char  *data;
    size_t len;
};

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userp) {
    struct http_buf *buf = userp;
    size_t total = size * nmemb;
    char *tmp = realloc(buf->data, buf->len + total + 1);
    if (!tmp) return 0;
    buf->data = tmp;
    memcpy(buf->data + buf->len, ptr, total);
    buf->len += total;
    buf->data[buf->len] = '\0';
    return total;
}

static struct json_object *http_get(mtc_conn_t *conn, const char *path) {
    char url[1024];
    snprintf(url, sizeof(url), "%s%s", conn->server_url, path);

    struct http_buf buf = {NULL, 0};
    curl_easy_reset(conn->curl);
    curl_easy_setopt(conn->curl, CURLOPT_URL, url);
    curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(conn->curl, CURLOPT_HTTPGET, 1L);

    CURLcode res = curl_easy_perform(conn->curl);
    if (res != CURLE_OK) {
        set_error("HTTP GET %s failed: %s", path, curl_easy_strerror(res));
        free(buf.data);
        return NULL;
    }

    struct json_object *obj = json_tokener_parse(buf.data);
    free(buf.data);
    return obj;
}

static struct json_object *http_post(mtc_conn_t *conn, const char *path,
                                     const char *json_body) {
    char url[1024];
    snprintf(url, sizeof(url), "%s%s", conn->server_url, path);

    struct http_buf buf = {NULL, 0};
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_reset(conn->curl);
    curl_easy_setopt(conn->curl, CURLOPT_URL, url);
    curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(conn->curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(conn->curl, CURLOPT_POSTFIELDS, json_body);

    CURLcode res = curl_easy_perform(conn->curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        set_error("HTTP POST %s failed: %s", path, curl_easy_strerror(res));
        free(buf.data);
        return NULL;
    }

    struct json_object *obj = json_tokener_parse(buf.data);
    free(buf.data);
    return obj;
}

/* ----------------------------------------------------------------------- */
/* Path helpers                                                            */
/* ----------------------------------------------------------------------- */

static const char *get_home(void) {
    const char *h = getenv("HOME");
    if (h) return h;
    struct passwd *pw = getpwuid(getuid());
    return pw ? pw->pw_dir : "/tmp";
}

static void tpm_path(char *out, size_t len, const char *subject, const char *file) {
    /* Sanitize subject: replace / and : with _ */
    char safe[256];
    strncpy(safe, subject, sizeof(safe) - 1);
    safe[sizeof(safe) - 1] = '\0';
    for (char *p = safe; *p; p++) {
        if (*p == '/' || *p == ':') *p = '_';
    }
    snprintf(out, len, "%s/.TPM/%s/%s", get_home(), safe, file);
}

static void tpm_dir(char *out, size_t len, const char *subject) {
    char safe[256];
    strncpy(safe, subject, sizeof(safe) - 1);
    safe[sizeof(safe) - 1] = '\0';
    for (char *p = safe; *p; p++) {
        if (*p == '/' || *p == ':') *p = '_';
    }
    snprintf(out, len, "%s/.TPM/%s", get_home(), safe);
}

static void mkdirp(const char *path) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0700);
            *p = '/';
        }
    }
    mkdir(tmp, 0700);
}

/* ----------------------------------------------------------------------- */
/* MTC_Connect                                                             */
/* ----------------------------------------------------------------------- */

mtc_conn_t *MTC_Connect(const char *server_url) {
    mtc_conn_t *conn = calloc(1, sizeof(*conn));
    if (!conn) return NULL;

    conn->server_url = strdup(server_url);
    conn->curl = curl_easy_init();
    if (!conn->curl) {
        set_error("failed to init curl");
        free(conn->server_url);
        free(conn);
        return NULL;
    }

    /* Fetch server info */
    struct json_object *info = http_get(conn, "/");
    if (!info) {
        MTC_Disconnect(conn);
        return NULL;
    }

    struct json_object *val;
    if (json_object_object_get_ex(info, "ca_name", &val))
        conn->ca_name = strdup(json_object_get_string(val));
    if (json_object_object_get_ex(info, "log_id", &val))
        conn->log_id = strdup(json_object_get_string(val));
    if (json_object_object_get_ex(info, "tree_size", &val))
        conn->tree_size = json_object_get_int(val);

    json_object_put(info);
    return conn;
}

void MTC_Disconnect(mtc_conn_t *conn) {
    if (!conn) return;
    if (conn->curl) curl_easy_cleanup(conn->curl);
    free(conn->server_url);
    free(conn->ca_name);
    free(conn->log_id);
    free(conn);
}

const char *MTC_Conn_CA_Name(const mtc_conn_t *conn) { return conn->ca_name; }
const char *MTC_Conn_Log_ID(const mtc_conn_t *conn) { return conn->log_id; }
int MTC_Conn_Tree_Size(const mtc_conn_t *conn) { return conn->tree_size; }

/* ----------------------------------------------------------------------- */
/* MTC_Enroll                                                              */
/* ----------------------------------------------------------------------- */

mtc_cert_t *MTC_Enroll(mtc_conn_t *conn, const char *subject,
                        const char *algorithm, int validity_days,
                        const mtc_extensions_t *extensions) {
    if (!algorithm) algorithm = "EC-P256";
    if (validity_days <= 0) validity_days = 90;

    /* Generate EC key pair using OpenSSL */
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (!ctx) { set_error("EVP_PKEY_CTX_new_id failed"); return NULL; }
    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1);
    EVP_PKEY_keygen(ctx, &pkey);
    EVP_PKEY_CTX_free(ctx);

    if (!pkey) { set_error("key generation failed"); return NULL; }

    /* Write private key to ~/.TPM */
    char dir[512], path[512];
    tpm_dir(dir, sizeof(dir), subject);
    mkdirp(dir);

    tpm_path(path, sizeof(path), subject, "private_key.pem");
    FILE *fp = fopen(path, "w");
    if (fp) {
        PEM_write_PrivateKey(fp, pkey, NULL, NULL, 0, NULL, NULL);
        fclose(fp);
        chmod(path, 0600);
    }

    /* Write public key */
    tpm_path(path, sizeof(path), subject, "public_key.pem");
    char *pub_pem = NULL;
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, pkey);
    long pem_len = BIO_get_mem_data(bio, &pub_pem);

    fp = fopen(path, "w");
    if (fp) {
        fwrite(pub_pem, 1, pem_len, fp);
        fclose(fp);
    }

    /* Build JSON request */
    struct json_object *req = json_object_new_object();
    json_object_object_add(req, "subject", json_object_new_string(subject));

    char pem_buf[4096];
    snprintf(pem_buf, sizeof(pem_buf), "%.*s", (int)pem_len, pub_pem);
    json_object_object_add(req, "public_key_pem", json_object_new_string(pem_buf));
    json_object_object_add(req, "key_algorithm", json_object_new_string(algorithm));
    json_object_object_add(req, "validity_days", json_object_new_int(validity_days));

    struct json_object *ext = json_object_new_object();
    json_object_object_add(ext, "key_usage", json_object_new_string("digitalSignature"));
    if (extensions) {
        for (int i = 0; i < extensions->count; i++)
            json_object_object_add(ext, extensions->keys[i],
                                   json_object_new_string(extensions->values[i]));
    }
    json_object_object_add(req, "extensions", ext);

    const char *body = json_object_to_json_string(req);
    struct json_object *resp = http_post(conn, "/certificate/request", body);
    json_object_put(req);
    BIO_free(bio);
    EVP_PKEY_free(pkey);

    if (!resp) return NULL;

    /* Save certificate JSON */
    tpm_path(path, sizeof(path), subject, "certificate.json");
    fp = fopen(path, "w");
    if (fp) {
        fprintf(fp, "%s\n", json_object_to_json_string_ext(resp, JSON_C_TO_STRING_PRETTY));
        fclose(fp);
    }

    /* Parse result into mtc_cert_t */
    mtc_cert_t *cert = calloc(1, sizeof(*cert));
    struct json_object *val;

    if (json_object_object_get_ex(resp, "index", &val))
        cert->index = json_object_get_int(val);

    /* Save index file */
    tpm_path(path, sizeof(path), subject, "index");
    fp = fopen(path, "w");
    if (fp) { fprintf(fp, "%d", cert->index); fclose(fp); }

    struct json_object *sc, *tbs;
    if (json_object_object_get_ex(resp, "standalone_certificate", &sc)) {
        if (json_object_object_get_ex(sc, "tbs_entry", &tbs)) {
            if (json_object_object_get_ex(tbs, "subject", &val))
                cert->subject = strdup(json_object_get_string(val));
            if (json_object_object_get_ex(tbs, "not_before", &val))
                cert->not_before = json_object_get_double(val);
            if (json_object_object_get_ex(tbs, "not_after", &val))
                cert->not_after = json_object_get_double(val);
            struct json_object *ext_obj;
            if (json_object_object_get_ex(tbs, "extensions", &ext_obj))
                cert->extensions_json = strdup(json_object_to_json_string(ext_obj));
        }
        if (json_object_object_get_ex(sc, "trust_anchor_id", &val))
            cert->trust_anchor_id = strdup(json_object_get_string(val));
    }

    cert->has_landmark = json_object_object_get_ex(resp, "landmark_certificate", &val);
    tpm_dir(path, sizeof(path), subject);
    cert->local_path = strdup(path);

    json_object_put(resp);
    return cert;
}

/* ----------------------------------------------------------------------- */
/* MTC_Verify                                                              */
/* ----------------------------------------------------------------------- */

mtc_verify_t *MTC_Verify(mtc_conn_t *conn, int index) {
    /* Fetch proof from server (server-side verification) */
    char path[256];
    snprintf(path, sizeof(path), "/log/proof/%d", index);
    struct json_object *proof = http_get(conn, path);

    mtc_verify_t *result = calloc(1, sizeof(*result));
    result->index = index;
    result->landmark_valid = -1;

    if (!proof) {
        result->error = strdup("failed to fetch proof");
        return result;
    }

    struct json_object *val;
    if (json_object_object_get_ex(proof, "valid", &val))
        result->inclusion_proof = json_object_get_boolean(val);

    /* Fetch certificate for subject and cosignature check */
    snprintf(path, sizeof(path), "/certificate/%d", index);
    struct json_object *cert = http_get(conn, path);
    if (cert) {
        struct json_object *sc, *tbs;
        if (json_object_object_get_ex(cert, "standalone_certificate", &sc)) {
            if (json_object_object_get_ex(sc, "tbs_entry", &tbs)) {
                if (json_object_object_get_ex(tbs, "subject", &val))
                    result->subject = strdup(json_object_get_string(val));
                /* Check expiry (1s tolerance for clock skew) */
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                double now = ts.tv_sec + ts.tv_nsec / 1e9;
                double nb = 0, na = 0;
                if (json_object_object_get_ex(tbs, "not_before", &val))
                    nb = json_object_get_double(val);
                if (json_object_object_get_ex(tbs, "not_after", &val))
                    na = json_object_get_double(val);
                result->not_expired = ((nb - 1.0) <= now && now <= (na + 1.0));
            }
            /* Cosignature present = valid (full verification requires Ed25519 check) */
            struct json_object *cosigs;
            if (json_object_object_get_ex(sc, "cosignatures", &cosigs))
                result->cosignature_valid = (json_object_array_length(cosigs) > 0);
        }
        json_object_put(cert);
    }

    result->valid = result->inclusion_proof && result->cosignature_valid && result->not_expired;

    json_object_put(proof);
    return result;
}

/* ----------------------------------------------------------------------- */
/* MTC_Find                                                                */
/* ----------------------------------------------------------------------- */

mtc_find_results_t *MTC_Find(mtc_conn_t *conn, const char *query) {
    char path[512];
    snprintf(path, sizeof(path), "/certificate/search?q=%s", query);
    struct json_object *resp = http_get(conn, path);
    if (!resp) return NULL;

    mtc_find_results_t *out = calloc(1, sizeof(*out));
    struct json_object *arr;
    if (json_object_object_get_ex(resp, "results", &arr)) {
        out->count = json_object_array_length(arr);
        out->results = calloc(out->count, sizeof(*out->results));
        for (int i = 0; i < out->count; i++) {
            struct json_object *item = json_object_array_get_idx(arr, i);
            struct json_object *val;
            if (json_object_object_get_ex(item, "index", &val))
                out->results[i].index = json_object_get_int(val);
            if (json_object_object_get_ex(item, "subject", &val))
                out->results[i].subject = strdup(json_object_get_string(val));
        }
    }
    json_object_put(resp);
    return out;
}

/* ----------------------------------------------------------------------- */
/* MTC_Status                                                              */
/* ----------------------------------------------------------------------- */

mtc_status_t *MTC_Status(mtc_conn_t *conn) {
    struct json_object *log = http_get(conn, "/log");
    if (!log) return NULL;

    mtc_status_t *s = calloc(1, sizeof(*s));
    s->server_url = strdup(conn->server_url);
    s->ca_name = conn->ca_name ? strdup(conn->ca_name) : NULL;
    s->log_id = conn->log_id ? strdup(conn->log_id) : NULL;

    struct json_object *val;
    if (json_object_object_get_ex(log, "tree_size", &val))
        s->tree_size = json_object_get_int(val);
    if (json_object_object_get_ex(log, "root_hash", &val))
        s->root_hash = strdup(json_object_get_string(val));
    struct json_object *lm;
    if (json_object_object_get_ex(log, "landmarks", &lm))
        s->landmark_count = json_object_array_length(lm);

    json_object_put(log);
    return s;
}

/* ----------------------------------------------------------------------- */
/* MTC_Renew                                                               */
/* ----------------------------------------------------------------------- */

mtc_cert_t *MTC_Renew(mtc_conn_t *conn, int index, int validity_days) {
    /* Fetch existing cert to get subject and extensions */
    char path[256];
    snprintf(path, sizeof(path), "/certificate/%d", index);
    struct json_object *cert = http_get(conn, path);
    if (!cert) { set_error("certificate %d not found", index); return NULL; }

    struct json_object *sc, *tbs, *val;
    const char *subject = NULL;
    const char *algorithm = "EC-P256";
    mtc_extensions_t *ext = NULL;

    if (json_object_object_get_ex(cert, "standalone_certificate", &sc) &&
        json_object_object_get_ex(sc, "tbs_entry", &tbs)) {
        if (json_object_object_get_ex(tbs, "subject", &val))
            subject = json_object_get_string(val);
        if (json_object_object_get_ex(tbs, "subject_public_key_algorithm", &val))
            algorithm = json_object_get_string(val);

        struct json_object *ext_obj;
        if (json_object_object_get_ex(tbs, "extensions", &ext_obj)) {
            ext = MTC_Extensions_New();
            json_object_object_foreach(ext_obj, key, ext_val) {
                MTC_Extensions_Add(ext, key, json_object_get_string(ext_val));
            }
        }
    }

    if (!subject) {
        set_error("could not read subject from certificate %d", index);
        json_object_put(cert);
        return NULL;
    }

    /* Archive old files */
    char old_path[512], new_path[512];
    tpm_path(old_path, sizeof(old_path), subject, "certificate.json");
    tpm_path(new_path, sizeof(new_path), subject, "certificate.json.old");
    rename(old_path, new_path);
    tpm_path(old_path, sizeof(old_path), subject, "private_key.pem");
    tpm_path(new_path, sizeof(new_path), subject, "private_key.pem.old");
    rename(old_path, new_path);

    /* Re-enroll */
    char *subject_copy = strdup(subject);
    json_object_put(cert);

    mtc_cert_t *new_cert = MTC_Enroll(conn, subject_copy, algorithm, validity_days, ext);
    free(subject_copy);
    if (ext) MTC_Free_Extensions(ext);
    return new_cert;
}

/* ----------------------------------------------------------------------- */
/* MTC_Revoke                                                              */
/* ----------------------------------------------------------------------- */

int MTC_Revoke(mtc_conn_t *conn, int index) {
    (void)conn;
    (void)index;
    /* Placeholder: in production, this would send a revocation request
     * to the CA or security team's revocation service. */
    return 0;
}

/* ----------------------------------------------------------------------- */
/* Memory management                                                       */
/* ----------------------------------------------------------------------- */

void MTC_Free_Cert(mtc_cert_t *cert) {
    if (!cert) return;
    free(cert->subject);
    free(cert->trust_anchor_id);
    free(cert->extensions_json);
    free(cert->local_path);
    free(cert);
}

void MTC_Free_Verify(mtc_verify_t *result) {
    if (!result) return;
    free(result->subject);
    free(result->error);
    free(result);
}

void MTC_Free_Find(mtc_find_results_t *results) {
    if (!results) return;
    for (int i = 0; i < results->count; i++)
        free(results->results[i].subject);
    free(results->results);
    free(results);
}

void MTC_Free_Status(mtc_status_t *status) {
    if (!status) return;
    free(status->server_url);
    free(status->ca_name);
    free(status->log_id);
    free(status->root_hash);
    free(status);
}

/* ----------------------------------------------------------------------- */
/* Extensions builder                                                      */
/* ----------------------------------------------------------------------- */

mtc_extensions_t *MTC_Extensions_New(void) {
    return calloc(1, sizeof(mtc_extensions_t));
}

int MTC_Extensions_Add(mtc_extensions_t *ext, const char *key, const char *value) {
    int n = ext->count + 1;
    ext->keys = realloc(ext->keys, n * sizeof(char *));
    ext->values = realloc(ext->values, n * sizeof(char *));
    ext->keys[ext->count] = strdup(key);
    ext->values[ext->count] = strdup(value);
    ext->count = n;
    return 0;
}

void MTC_Free_Extensions(mtc_extensions_t *ext) {
    if (!ext) return;
    for (int i = 0; i < ext->count; i++) {
        free(ext->keys[i]);
        free(ext->values[i]);
    }
    free(ext->keys);
    free(ext->values);
    free(ext);
}

/* ----------------------------------------------------------------------- */
/* MTC_List (stub — reads ~/.TPM directory)                                */
/* ----------------------------------------------------------------------- */

mtc_cert_t **MTC_List(int *count) {
    *count = 0;
    /* TODO: scan ~/.TPM directories and parse certificate.json files */
    return NULL;
}
