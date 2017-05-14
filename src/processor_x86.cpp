// This file is a part of Julia. License is MIT: https://julialang.org/license

// X86 specific processor detection and dispatch

// CPUID

extern "C" JL_DLLEXPORT void jl_cpuid(int32_t CPUInfo[4], int32_t InfoType)
{
#if defined _MSC_VER
    __cpuid(CPUInfo, InfoType);
#else
    asm volatile (
#if defined(__i386__) && defined(__PIC__)
        "xchg %%ebx, %%esi;"
        "cpuid;"
        "xchg %%esi, %%ebx;" :
        "=S" (CPUInfo[1]),
#else
        "cpuid" :
        "=b" (CPUInfo[1]),
#endif
        "=a" (CPUInfo[0]),
        "=c" (CPUInfo[2]),
        "=d" (CPUInfo[3]) :
        "a" (InfoType)
        );
#endif
}

extern "C" JL_DLLEXPORT void jl_cpuidex(int32_t CPUInfo[4], int32_t InfoType, int32_t subInfoType)
{
#if defined _MSC_VER
    __cpuidex(CPUInfo, InfoType, subInfoType);
#else
    asm volatile (
#if defined(__i386__) && defined(__PIC__)
        "xchg %%ebx, %%esi;"
        "cpuid;"
        "xchg %%esi, %%ebx;" :
        "=S" (CPUInfo[1]),
#else
        "cpuid" :
        "=b" (CPUInfo[1]),
#endif
        "=a" (CPUInfo[0]),
        "=c" (CPUInfo[2]),
        "=d" (CPUInfo[3]) :
        "a" (InfoType),
        "c" (subInfoType)
        );
#endif
}

namespace X86 {

// For CPU model and feature detection on X86

const int SIG_INTEL = 0x756e6547; // Genu
const int SIG_AMD = 0x68747541; // Auth

static uint64_t get_xcr0(void)
{
#if defined _MSC_VER
    return _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
#else
    uint32_t eax, edx;
    asm volatile ("xgetbv" : "=a" (eax), "=d" (edx) : "c" (0));
    return (uint64_t(edx) << 32) | eax;
#endif
}

static CPU get_intel_processor_name(uint32_t family, uint32_t model, uint32_t brand_id,
                                    const uint32_t *features)
{
    if (brand_id != 0)
        return CPU::generic;
    switch (family) {
    case 3:
    case 4:
    case 5:
        return CPU::generic;
    case 6:
        switch (model) {
        case 0x01: // Pentium Pro processor
        case 0x03: // Intel Pentium II OverDrive processor, Pentium II processor, model 03
        case 0x05: // Pentium II processor, model 05, Pentium II Xeon processor,
            // model 05, and Intel Celeron processor, model 05
        case 0x06: // Celeron processor, model 06
        case 0x07: // Pentium III processor, model 07, and Pentium III Xeon processor, model 07
        case 0x08: // Pentium III processor, model 08, Pentium III Xeon processor,
            // model 08, and Celeron processor, model 08
        case 0x0a: // Pentium III Xeon processor, model 0Ah
        case 0x0b: // Pentium III processor, model 0Bh
        case 0x09: // Intel Pentium M processor, Intel Celeron M processor model 09.
        case 0x0d: // Intel Pentium M processor, Intel Celeron M processor, model
            // 0Dh. All processors are manufactured using the 90 nm process.
        case 0x15: // Intel EP80579 Integrated Processor and Intel EP80579
            // Integrated Processor with Intel QuickAssist Technology
            return CPU::generic;
        case 0x0e: // Intel Core Duo processor, Intel Core Solo processor, model
            // 0Eh. All processors are manufactured using the 65 nm process.
            return CPU::intel_yonah;
        case 0x0f: // Intel Core 2 Duo processor, Intel Core 2 Duo mobile
            // processor, Intel Core 2 Quad processor, Intel Core 2 Quad
            // mobile processor, Intel Core 2 Extreme processor, Intel
            // Pentium Dual-Core processor, Intel Xeon processor, model
            // 0Fh. All processors are manufactured using the 65 nm process.
        case 0x16: // Intel Celeron processor model 16h. All processors are
            // manufactured using the 65 nm process
            return CPU::intel_core2;
        case 0x17: // Intel Core 2 Extreme processor, Intel Xeon processor, model
            // 17h. All processors are manufactured using the 45 nm process.
            //
            // 45nm: Penryn , Wolfdale, Yorkfield (XE)
        case 0x1d: // Intel Xeon processor MP. All processors are manufactured using
            // the 45 nm process.
            return CPU::intel_core2_penryn;
        case 0x1a: // Intel Core i7 processor and Intel Xeon processor. All
            // processors are manufactured using the 45 nm process.
        case 0x1e: // Intel(R) Core(TM) i7 CPU         870  @ 2.93GHz.
            // As found in a Summer 2010 model iMac.
        case 0x1f:
        case 0x2e: // Nehalem EX
            return CPU::intel_corei7_nehalem;
        case 0x25: // Intel Core i7, laptop version.
        case 0x2c: // Intel Core i7 processor and Intel Xeon processor. All
            // processors are manufactured using the 32 nm process.
        case 0x2f: // Westmere EX
            return CPU::intel_corei7_westmere;
        case 0x2a: // Intel Core i7 processor. All processors are manufactured
            // using the 32 nm process.
        case 0x2d:
            return CPU::intel_corei7_sandybridge;
        case 0x3a:
        case 0x3e: // Ivy Bridge EP
            return CPU::intel_corei7_ivybridge;

            // Haswell:
        case 0x3c:
        case 0x3f:
        case 0x45:
        case 0x46:
            return CPU::intel_corei7_haswell;

            // Broadwell:
        case 0x3d:
        case 0x47:
        case 0x4f:
        case 0x56:
            return CPU::intel_corei7_broadwell;

            // Skylake:
        case 0x4e: // Skylake mobile
        case 0x5e: // Skylake desktop
        case 0x8e: // Kaby Lake mobile
        case 0x9e: // Kaby Lake desktop
            return CPU::intel_corei7_skylake;

            // Skylake Xeon:
        case 0x55:
            if (test_nbit(features, Feature::avx512f))
                return CPU::intel_corei7_skylake_avx512;
            return CPU::intel_corei7_skylake;

        case 0x1c: // Most 45 nm Intel Atom processors
        case 0x26: // 45 nm Atom Lincroft
        case 0x27: // 32 nm Atom Medfield
        case 0x35: // 32 nm Atom Midview
        case 0x36: // 32 nm Atom Midview
            return CPU::intel_atom_bonnell;

            // Atom Silvermont codes from the Intel software optimization guide.
        case 0x37:
        case 0x4a:
        case 0x4d:
        case 0x5a:
        case 0x5d:
        case 0x4c: // really airmont
            return CPU::intel_atom_silvermont;

        case 0x57:
            return CPU::intel_knights_landing;

        default:
            return CPU::generic;
        }
        break;
    case 15: {
        switch (model) {
        case 0: // Pentium 4 processor, Intel Xeon processor. All processors are
            // model 00h and manufactured using the 0.18 micron process.
        case 1: // Pentium 4 processor, Intel Xeon processor, Intel Xeon
            // processor MP, and Intel Celeron processor. All processors are
            // model 01h and manufactured using the 0.18 micron process.
        case 2: // Pentium 4 processor, Mobile Intel Pentium 4 processor - M,
            // Intel Xeon processor, Intel Xeon processor MP, Intel Celeron
            // processor, and Mobile Intel Celeron processor. All processors
            // are model 02h and manufactured using the 0.13 micron process.
        default:
            return CPU::generic;

        case 3: // Pentium 4 processor, Intel Xeon processor, Intel Celeron D
            // processor. All processors are model 03h and manufactured using
            // the 90 nm process.
        case 4: // Pentium 4 processor, Pentium 4 processor Extreme Edition,
            // Pentium D processor, Intel Xeon processor, Intel Xeon
            // processor MP, Intel Celeron D processor. All processors are
            // model 04h and manufactured using the 90 nm process.
        case 6: // Pentium 4 processor, Pentium D processor, Pentium processor
            // Extreme Edition, Intel Xeon processor, Intel Xeon processor
            // MP, Intel Celeron D processor. All processors are model 06h
            // and manufactured using the 65 nm process.
#ifdef _CPU_X86_64_
            return CPU::intel_nocona;
#else
            return CPU::intel_prescott;
#endif
        }
    }
    default:
        break; /*"generic"*/
    }
    return CPU::generic;
}

static CPU get_amd_processor_name(uint32_t family, uint32_t model, const uint32_t *features)
{
    switch (family) {
    case 4:
    case 5:
    case 6:
    default:
        return CPU::generic;
    case 15:
        if (test_nbit(features, Feature::sse3))
            return CPU::amd_k8_sse3;
        switch (model) {
        case 1:
            return CPU::amd_opteron;
        case 5:
            return CPU::amd_athlon_fx;
        default:
            return CPU::amd_athlon_64;
        }
    case 16:
        switch (model) {
        case 2:
            return CPU::amd_barcelona;
        case 4:
        case 8:
        default:
            return CPU::amd_fam10h;
        }
    case 20:
        return CPU::amd_btver1;
    case 21:
        if (!test_nbit(features, Feature::avx))
            return CPU::amd_btver1;
        if (model >= 0x50 && model <= 0x6f)
            return CPU::amd_bdver4;
        if (model >= 0x30 && model <= 0x3f)
            return CPU::amd_bdver3;
        if (model >= 0x10 && model <= 0x1f)
            return CPU::amd_bdver2;
        if (model <= 0x0f)
            return CPU::amd_bdver1;
        return CPU::amd_btver1; // fallback
    case 22:
        if (!test_nbit(features, Feature::avx))
            return CPU::amd_btver1;
        return CPU::amd_btver2;
    case 23:
        if (test_nbit(features, Feature::adx))
            return CPU::amd_znver1;
        return CPU::amd_btver1;
    }
}

template<typename T>
static inline void features_disable_avx512(T &features)
{
    using namespace Feature;
    unset_bits(features, avx512f, avx512dq, avx512ifma, avx512pf, avx512er, avx512cd,
               avx512bw, avx512vl, avx512vbmi);
}

template<typename T>
static inline void features_disable_avx(T &features)
{
    using namespace Feature;
    unset_bits(features, avx, Feature::fma, f16c, xsave, avx2, xop, fma4,
               xsaveopt, xsavec, xsaves);
}

const auto host_cpu = [] {
    FeatureList<feature_sz> features = {};

    int32_t info0[4];
    jl_cpuid(info0, 0);
    int32_t info1[4];
    jl_cpuid(info1, 1);

    uint32_t maxleaf = info0[0];
    auto vendor = info0[1];
    auto brand_id = info1[1] & 0xff;

    auto family = (info1[0] >> 8) & 0xf; // Bits 8 - 11
    auto model = (info1[0] >> 4) & 0xf;  // Bits 4 - 7
    if (family == 6 || family == 0xf) {
        if (family == 0xf)
            // Examine extended family ID if family ID is F.
            family += (info1[0] >> 20) & 0xff; // Bits 20 - 27
        // Examine extended model ID if family ID is 6 or F.
        model += ((info1[0] >> 16) & 0xf) << 4; // Bits 16 - 19
    }

    // Fill in the features
    features[0] = info1[2];
    features[1] = info1[3];
    if (maxleaf >= 7) {
        int32_t info7[4];
        jl_cpuidex(info7, 7, 0);
        features[2] = info7[1];
        features[3] = info7[2];
        features[4] = info7[3];
    }
    int32_t infoex0[4];
    jl_cpuid(infoex0, 0x80000000);
    uint32_t maxexleaf = infoex0[0];
    if (maxexleaf >= 0x80000001) {
        int32_t infoex1[4];
        jl_cpuid(infoex1, 0x80000001);
        features[5] = infoex1[2];
        features[6] = infoex1[3];
    }
    if (maxleaf >= 0xd) {
        int32_t infod[4];
        jl_cpuidex(infod, 0xd, 0x1);
        features[7] = infod[0];
    }
    if (maxexleaf >= 0x80000008) {
        int32_t infoex8[4];
        jl_cpuidex(infoex8, 0x80000008, 0);
        features[8] = infoex8[1];
    }

    // Fix up AVX bits to account for OS support and match LLVM model
    uint64_t xcr0 = 0;
    const uint32_t avx_mask = (1 << 27) | (1 << 28);
    bool hasavx = test_bits(features[1], avx_mask);
    if (hasavx) {
        xcr0 = get_xcr0();
        hasavx = test_bits(xcr0, 0x6);
    }
    unset_bits(features, 32 + 27);
    if (!hasavx)
        features_disable_avx(features);
    bool hasavx512save = hasavx && test_bits(xcr0, 0xe0);
    if (!hasavx512save)
        features_disable_avx512(features);
    // Ignore feature bits that we are not interested in.
    mask_features(feature_masks, &features[0]);

    uint32_t cpu;
    if (vendor == SIG_INTEL) {
        cpu = uint32_t(get_intel_processor_name(family, model, brand_id, &features[0]));
    }
    else if (vendor == SIG_AMD) {
        cpu = uint32_t(get_amd_processor_name(family, model, &features[0]));
    }
    else {
        cpu = uint32_t(CPU::generic);
    }

    return std::make_pair(cpu, features);
}();

static inline const CPUSpec<CPU,feature_sz> *find_cpu(uint32_t cpu)
{
    return ::find_cpu(cpu, cpus, ncpu_names);
}

static inline const CPUSpec<CPU,feature_sz> *find_cpu(const char *name)
{
    return ::find_cpu(name, cpus, ncpu_names);
}

static inline const char *find_cpu_name(uint32_t cpu)
{
    return ::find_cpu_name(cpu, cpus, ncpu_names);
}

static inline CPU find_cpu_id(const char *name)
{
    return ::find_cpu_id(name, cpus, ncpu_names);
}

static inline const std::string &host_cpu_name()
{
    static std::string name =
        (CPU)host_cpu.first != CPU::generic ?
        std::string(find_cpu_name(host_cpu.first)) :
        jl_get_cpu_name_llvm();
    return name;
}

static inline const char *normalize_cpu_name(const char *name)
{
    if (strcmp(name, "atom") == 0)
        return "bonnell";
    if (strcmp(name, "slm") == 0)
        return "silvermont";
    if (strcmp(name, "corei7") == 0)
        return "nehalem";
    if (strcmp(name, "corei7-avx") == 0)
        return "sandybridge";
    if (strcmp(name, "core-avx-i") == 0)
        return "ivybridge";
    if (strcmp(name, "core-avx2") == 0)
        return "haswell";
    if (strcmp(name, "skx") == 0)
        return "skylake-avx512";
    if (strcmp(name, "pentium4") == 0)
        return "generic";
    return nullptr;
}

template<size_t n>
static inline void enable_depends(FeatureList<n> &features)
{
    ::enable_depends(features, Feature::deps, sizeof(Feature::deps) / sizeof(FeatureDep));
}

template<size_t n>
static inline void disable_depends(FeatureList<n> &features)
{
    ::disable_depends(features, Feature::deps, sizeof(Feature::deps) / sizeof(FeatureDep));
}

static const std::vector<TargetArg<feature_sz>> &get_cmdline_targets(void)
{
    auto feature_cb = [] (const char *str, size_t len, FeatureList<feature_sz> &list) {
        auto fbit = find_feature_bit(feature_names, nfeature_names, str, len);
        if (fbit == (uint32_t)-1)
            return false;
        set_bit(list, fbit, true);
        return true;
    };
    auto &targets = ::get_cmdline_targets<feature_sz>(feature_cb);
    for (auto &t: targets) {
        if (auto nname = normalize_cpu_name(t.name.c_str())) {
            t.name = nname;
        }
    }
    return targets;
}

static std::vector<TargetData<feature_sz>> jit_targets;

static TargetData<feature_sz> host_target_data(void)
{
    return TargetData<feature_sz>{host_cpu_name(), "", host_cpu.second, 0};
}

static TargetData<feature_sz> arg_target_data(const TargetArg<feature_sz> &arg, bool require_host)
{
    TargetData<feature_sz> res = arg;
    const FeatureList<feature_sz> *cpu_features = nullptr;
    if (res.name == "native") {
        res.name = host_cpu_name();
        cpu_features = &host_cpu.second;
    }
    else if (auto spec = find_cpu(res.name.c_str())) {
        cpu_features = &spec->features;
    }
    else {
        res.flags |= JL_TARGET_INCOMPLETE_FEATURE;
    }
    if (cpu_features) {
        for (size_t i = 0; i < feature_sz; i++) {
            res.features[i] |= (*cpu_features)[i];
        }
    }
    else {
        // The feature array will only contain enabled feature and not disabled features
        // Add disabled features to ext_features
        auto &ext_features = res.ext_features;
        for (auto &fename: feature_names) {
            if (test_nbit(arg.disable, fename.bit) ||
                (require_host && !test_nbit(host_cpu.second, fename.bit))) {
                if (!ext_features.empty())
                    ext_features.push_back(',');
                ext_features.append(std::string("-") + fename.name);
            }
        }
    }
    enable_depends(res.features);
    for (size_t i = 0; i < feature_sz; i++)
        res.features[i] &= ~arg.disable[i];
    if (require_host) {
        for (size_t i = 0; i < feature_sz; i++) {
            res.features[i] &= host_cpu.second[i];
        }
    }
    disable_depends(res.features);
    return res;
}

static int max_vector_size(const FeatureList<feature_sz> &features)
{
    if (test_nbit(features, Feature::avx512f))
        return 64;
    if (test_nbit(features, Feature::avx))
        return 32;
    // SSE is required
    return 16;
}

static uint32_t sysimg_init_cb(const void *id)
{
    // First see what target is requested for the JIT.
    auto &cmdline = get_cmdline_targets();
    TargetData<feature_sz> target;
    // It's unclear what does specifying multiple target when not generating
    // sysimg means. Make it an error for now.
    if (cmdline.size() > 1) {
        jl_error("More than one command line CPU targets specified when"
                 "not generating sysimg");
    }
    else if (cmdline.empty()) {
        target = host_target_data();
    }
    else {
        auto &targetarg = cmdline[0];
        if (targetarg.flags & JL_TARGET_CLONE_ALL)
            jl_error("`clone_all` feature specified when not generating sysimg.");
        target = arg_target_data(targetarg, true);
    }
    // Then find the best match in the sysimg
    uint32_t best_idx = (uint32_t)-1;
    bool match_name = false;
    int vreg_size = 0;
    int feature_size = 0;
    auto sysimg = deserialize_target_data<feature_sz>((const uint8_t*)id);
    for (uint32_t i = 0; i < sysimg.size(); i++) {
        auto &imgt = sysimg[i];
        if (!features_le(imgt.features, target.features))
            continue;
        if (imgt.name == target.name) {
            if (!match_name) {
                match_name = true;
                vreg_size = 0;
                feature_size = 0;
            }
        }
        else if (match_name) {
            continue;
        }
        int new_vsz = max_vector_size(imgt.features);
        if (vreg_size > new_vsz)
            continue;
        int new_feature_size = count_bits(imgt.features);
        if (vreg_size < new_vsz) {
            best_idx = i;
            vreg_size = new_vsz;
            feature_size = new_feature_size;
            continue;
        }
        if (new_feature_size <= feature_size)
            continue;
        best_idx = i;
        feature_size = new_feature_size;
    }
    if (best_idx == (uint32_t)-1)
        jl_error("Unable to find compatible target in sysimg.");
    // Now we've decided on which sysimg version to use.
    // Make sure the JIT target is compatible with it and save the JIT target.
    if (vreg_size != max_vector_size(target.features) &&
        (sysimg[best_idx].flags & JL_TARGET_VEC_CALL)) {
        if (vreg_size < 64) {
            features_disable_avx512(target.features);
        }
        if (vreg_size < 32) {
            features_disable_avx(target.features);
        }
    }
    jit_targets.push_back(std::move(target));
    return best_idx;
}

static void ensure_jit_target(bool imaging)
{
    if (!jit_targets.empty())
        return;
    auto &cmdline = get_cmdline_targets();
    if (cmdline.size() > 1 && !imaging) {
        jl_error("More than one command line CPU targets specified when"
                 "not generating sysimg");
    }
    else if (cmdline.empty()) {
        jit_targets.push_back(host_target_data());
        return;
    }
    for (auto &arg: cmdline) {
        auto data = arg_target_data(arg, jit_targets.empty());
        jit_targets.push_back(std::move(data));
    }
    auto ntargets = jit_targets.size();
    auto &features0 = jit_targets[0].features;
    // Now decide the clone condition.
    for (size_t i = 1; i < ntargets; i++) {
        auto &t = jit_targets[i];
        if (t.flags & JL_TARGET_CLONE_ALL)
            continue;
        // The most useful one in general...
        t.flags |= JL_TARGET_CLONE_LOOP;
        // Special case for KNL since it's so different
        if (t.name == "knl" && jit_targets[0].name != "knl")
            t.flags |= JL_TARGET_CLONE_ALL;
        static constexpr uint32_t clone_math[] = {Feature::fma, Feature::fma4};
        static constexpr uint32_t clone_simd[] = {Feature::sse3, Feature::ssse3,
                                                  Feature::sse41, Feature::sse42,
                                                  Feature::avx, Feature::avx2,
                                                  Feature::sse4a, Feature::avx512f,
                                                  Feature::avx512dq, Feature::avx512ifma,
                                                  Feature::avx512pf, Feature::avx512er,
                                                  Feature::avx512cd, Feature::avx512bw,
                                                  Feature::avx512vl, Feature::avx512vbmi,
                                                  Feature::avx512vpopcntdq};
        for (auto fe: clone_math) {
            if (!test_nbit(features0, fe) && test_nbit(t.features, fe)) {
                t.flags |= JL_TARGET_CLONE_MATH;
                break;
            }
        }
        for (auto fe: clone_simd) {
            if (!test_nbit(features0, fe) && test_nbit(t.features, fe)) {
                t.flags |= JL_TARGET_CLONE_SIMD;
                break;
            }
        }
    }
}

static std::pair<std::string,std::vector<std::string>>
get_llvm_target_noext(const TargetData<feature_sz> &data, uint32_t llvmver)
{
    std::string name = data.name;
    while (auto *spec = find_cpu(name.c_str())) {
        if (spec->llvmver <= llvmver)
            break;
        name = find_cpu_name((uint32_t)spec->fallback);
    }
    std::vector<std::string> features;
    for (auto &fename: feature_names) {
        if (fename.llvmver > llvmver)
            continue;
        if (test_nbit(data.features, fename.bit)) {
            features.insert(features.begin(), fename.name);
        }
        else if (!(data.flags | JL_TARGET_INCOMPLETE_FEATURE)) {
            features.push_back(std::string("-") + fename.name);
        }
    }
    features.push_back("sse2");
    return std::make_pair(std::move(name), std::move(features));
}

static std::pair<std::string,std::vector<std::string>>
get_llvm_target_vec(const TargetData<feature_sz> &data, uint32_t llvmver)
{
    auto res0 = get_llvm_target_noext(data, llvmver);
    append_ext_features(res0.second, data.ext_features);
    return res0;
}

static std::pair<std::string,std::string>
get_llvm_target_str(const TargetData<feature_sz> &data, uint32_t llvmver)
{
    auto res0 = get_llvm_target_noext(data, llvmver);
    auto features = join_feature_strs(res0.second);
    append_ext_features(features, data.ext_features);
    return std::make_pair(std::move(res0.first), std::move(features));
}

}

using namespace X86;

JL_DLLEXPORT void jl_dump_host_cpu(void)
{
    dump_cpu_spec(host_cpu.first, host_cpu.second, feature_names, nfeature_names,
                  cpus, ncpu_names);
}

JL_DLLEXPORT jl_value_t *jl_get_cpu_name(void)
{
    return jl_cstr_to_string(host_cpu_name().c_str());
}

jl_sysimg_fptrs_t jl_init_processor_sysimg(void *hdl)
{
    if (!jit_targets.empty())
        jl_error("JIT targets already initialized");
    return parse_sysimg(hdl, sysimg_init_cb);
}

std::pair<std::string,std::vector<std::string>> jl_get_llvm_target(bool imaging, uint32_t llvmver)
{
    ensure_jit_target(imaging);
    return get_llvm_target_vec(jit_targets[0], llvmver);
}

std::pair<std::string,std::string> jl_get_llvm_disasm_target(uint32_t llvmver)
{
    return get_llvm_target_str(TargetData<feature_sz>{"generic", "", feature_masks, 0}, llvmver);
}

std::vector<jl_target_spec_t> jl_get_llvm_clone_targets(uint32_t llvmver)
{
    if (jit_targets.empty())
        jl_error("JIT targets not initialized");
    std::vector<jl_target_spec_t> res;
    for (auto &target: jit_targets) {
        auto features = target.features;
        for (auto &fename: feature_names) {
            if (fename.llvmver > llvmver) {
                unset_bits(features, fename.bit);
            }
        }
        X86::disable_depends(features);
        jl_target_spec_t ele;
        std::tie(ele.cpu_name, ele.cpu_features) = get_llvm_target_str(target, llvmver);
        ele.data = serialize_target_data(target.name.c_str(), features,
                                         target.ext_features.c_str());
        ele.cond = target.flags;
        res.push_back(ele);
    }
    return res;
}

extern "C" int jl_test_cpu_feature(jl_cpu_feature_t feature)
{
    if (feature >= 32 * feature_sz)
        return 0;
    return test_nbit(&host_cpu.second[0], feature);
}

// -- set/clear the FZ/DAZ flags on x86 & x86-64 --

// Cache of information recovered from `cpuid` since executing `cpuid` it at runtime is slow.
static uint32_t subnormal_flags = [] {
    int32_t info[4];
    jl_cpuid(info, 0);
    if (info[0] >= 1) {
        jl_cpuid(info, 1);
        if (info[3] & (1 << 26)) {
            // SSE2 supports both FZ and DAZ
            return 0x00008040;
        }
        else if (info[3] & (1 << 25)) {
            // SSE supports only the FZ flag
            return 0x00008000;
        }
    }
    return 0;
}();

// Returns non-zero if subnormals go to 0; zero otherwise.
extern "C" JL_DLLEXPORT int32_t jl_get_zero_subnormals(void)
{
    return _mm_getcsr() & subnormal_flags;
}

// Return zero on success, non-zero on failure.
extern "C" JL_DLLEXPORT int32_t jl_set_zero_subnormals(int8_t isZero)
{
    uint32_t flags = subnormal_flags;
    if (flags) {
        uint32_t state = _mm_getcsr();
        if (isZero)
            state |= flags;
        else
            state &= ~flags;
        _mm_setcsr(state);
        return 0;
    }
    else {
        // Report a failure only if user is trying to enable FTZ/DAZ.
        return isZero;
    }
}