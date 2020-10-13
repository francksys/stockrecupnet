
extern char            guest_download[254 + sizeof("/store/")];
extern char            guest_download_ok[254 + sizeof("/store/")];

int
srn_handle(int suser, struct request_line *rline,
           struct hdr_nv hdrnv[MAX_HEADERS],
           char *usraddr);
