#undef SYSROOT_SUFFIX_SPEC
#define SYSROOT_SUFFIX_SPEC "" \
"%{ml:" \
  "%{m1:/ml/m1;" \
  "m2|m3|m4-nofpu|m4-100-nofpu|m4-200-nofpu|m4-400|m4-500|m4-340|m4-300-nofpu|m4al|m4a-nofpu|m2a-nofpu:/ml/m2;" \
  "m2e|m3e:/ml/m2e;" \
  "m4|m4-100|m4-200|m4-300|m4a:/ml/m4;" \
  "m4-single|m4-100-single|m4-200-single|m4-300-single|m4a-single:/ml/m4-single;" \
  "m4-single-only|m4-100-single-only|m4-200-single-only|m4-300-single-only|m4a-single-only:/ml/m4-single-only;" \
  "m2a:/ml/m2a;" \
  "m2a-single|m2a-single-only:/ml/m2a-single;" \
  ":/ml};" \
":" \
  "%{m1:/m1;" \
  "m2|m3|m4-nofpu|m4-100-nofpu|m4-200-nofpu|m4-400|m4-500|m4-340|m4-300-nofpu|m4al|m4a-nofpu|m2a-nofpu:/m2;" \
  "m2e|m3e:/m2e;" \
  "m4|m4-100|m4-200|m4-300|m4a:/m4;" \
  "m4-single|m4-100-single|m4-200-single|m4-300-single|m4a-single:/m4-single;" \
  "m4-single-only|m4-100-single-only|m4-200-single-only|m4-300-single-only|m4a-single-only:/m4-single-only;" \
  "m2a:/m2a;" \
  "m2a-single|m2a-single-only:/m2a-single;" \
  ":}}"
