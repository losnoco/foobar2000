#ifdef __cplusplus
extern "C" {
#endif

typedef struct KMP_CTX_TAG KMP_CTX;

KMP_CTX *Open(const char *path, unsigned size, unsigned srate, unsigned nch);
void Close(KMP_CTX *ctx);
unsigned Write(KMP_CTX *ctx, void *buf, unsigned smp);
unsigned WriteSkip(KMP_CTX *ctx, unsigned smp);
unsigned SetPosition(KMP_CTX *ctx, unsigned ms);
int GetLoopFlag(KMP_CTX *ctx);
int GetSeekableFlag(KMP_CTX *ctx);
int GetMultisongFlag(KMP_CTX *pctx);
unsigned GetFrequency(KMP_CTX *ctx);
unsigned GetChannels(KMP_CTX *ctx);
unsigned GetSamplebits(KMP_CTX *ctx);
unsigned GetLength(KMP_CTX *ctx);
unsigned GetUnitSamples(KMP_CTX *ctx);

const char **GetPluginInfo(void);
void Init(void);
void Deinit(void);

#ifdef __cplusplus
}
#endif
