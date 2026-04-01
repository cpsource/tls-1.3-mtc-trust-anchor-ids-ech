/*
 * MTC API usage example.
 *
 * Build:  make
 * Run:    ./example
 * (Requires the MTC server running on localhost:8443)
 */

#include <stdio.h>
#include "mtc.h"

int main(void) {
    /* Connect to server */
    printf("Connecting to MTC server...\n");
    mtc_conn_t *conn = MTC_Connect("http://localhost:8443");
    if (!conn) {
        fprintf(stderr, "Failed to connect: %s\n", MTC_Last_Error());
        return 1;
    }
    printf("  Connected to %s (log size: %d)\n\n",
           MTC_Conn_CA_Name(conn), MTC_Conn_Tree_Size(conn));

    /* Enroll */
    printf("Enrolling...\n");
    mtc_extensions_t *ext = MTC_Extensions_New();
    MTC_Extensions_Add(ext, "human_id", "Cal Page");
    MTC_Extensions_Add(ext, "app_instance", "truthorlieAccess");

    mtc_cert_t *cert = MTC_Enroll(conn,
        "urn:ajax-inc:app:truthorlieAccess-c",
        "EC-P256", 90, ext);
    MTC_Free_Extensions(ext);

    if (!cert) {
        fprintf(stderr, "Enroll failed: %s\n", MTC_Last_Error());
        MTC_Disconnect(conn);
        return 1;
    }
    printf("  Certificate #%d for %s\n", cert->index, cert->subject);
    printf("  Trust anchor: %s\n", cert->trust_anchor_id);
    printf("  Stored in: %s\n\n", cert->local_path);

    /* Verify */
    printf("Verifying...\n");
    mtc_verify_t *v = MTC_Verify(conn, cert->index);
    if (v) {
        printf("  Inclusion proof: %s\n", v->inclusion_proof ? "PASS" : "FAIL");
        printf("  Cosignature:     %s\n", v->cosignature_valid ? "PASS" : "FAIL");
        printf("  Not expired:     %s\n", v->not_expired ? "PASS" : "FAIL");
        printf("  Overall:         %s\n\n", v->valid ? "VALID" : "INVALID");
        MTC_Free_Verify(v);
    }

    /* Find */
    printf("Searching for 'truthorlie'...\n");
    mtc_find_results_t *found = MTC_Find(conn, "truthorlie");
    if (found) {
        printf("  Found %d result(s)\n", found->count);
        for (int i = 0; i < found->count; i++)
            printf("    #%d  %s\n", found->results[i].index, found->results[i].subject);
        MTC_Free_Find(found);
    }

    /* Status */
    printf("\nServer status:\n");
    mtc_status_t *s = MTC_Status(conn);
    if (s) {
        printf("  CA: %s, Log: %s\n", s->ca_name, s->log_id);
        printf("  Tree size: %d, Landmarks: %d\n", s->tree_size, s->landmark_count);
        MTC_Free_Status(s);
    }

    /* Cleanup */
    MTC_Free_Cert(cert);
    MTC_Disconnect(conn);
    printf("\nDone.\n");
    return 0;
}
