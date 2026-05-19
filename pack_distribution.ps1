# =============================================================================
# pack_distribution.ps1
# Script de empacotamento de distribuicao do TriDJs Stems
#
# Uso: .\pack_distribution.ps1
#      .\pack_distribution.ps1 -Config Debug
#      .\pack_distribution.ps1 -Config Release -Output "C:\Releases\v1.0"
# =============================================================================

param(
    [string]$Config  = "Release",
    [string]$Output  = ".\dist\TriDJs_Stems_v1.0"
)

$ErrorActionPreference = "Stop"
$LIBTORCH_DIR = "C:\TridjsStems\libtorch\lib"
$EXE_DIR      = ".\build\TriDJs_Separador_Stems_artefacts\$Config"
$EXE_NAME     = "TriDJs Stems.exe"
$RESOURCES    = @("logo.png", "splash.png")

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  TriDJs Stems - Empacotador de Distribuicao" -ForegroundColor Cyan
Write-Host "  Config : $Config" -ForegroundColor Cyan
Write-Host "  Output : $Output" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# 1. Cria pasta de saida limpa
if (Test-Path $Output) {
    Remove-Item -Recurse -Force $Output
}
New-Item -ItemType Directory -Path $Output | Out-Null
New-Item -ItemType Directory -Path "$Output\resources" | Out-Null

# 2. Copia o executavel principal
$exePath = Join-Path $EXE_DIR $EXE_NAME
if (-not (Test-Path $exePath)) {
    Write-Host "[ERRO] Executavel nao encontrado: $exePath" -ForegroundColor Red
    Write-Host "       Execute o build antes de empacotar." -ForegroundColor Yellow
    exit 1
}
Copy-Item $exePath $Output
Write-Host "[OK] $EXE_NAME" -ForegroundColor Green

# 3. Copia recursos (logo, splash, modelo)
foreach ($res in $RESOURCES) {
    if (Test-Path ".\$res") {
        Copy-Item ".\$res" "$Output\resources\"
        Write-Host "[OK] Recurso: $res" -ForegroundColor Green
    }
}

# Modelo de IA (obrigatorio)
$modelSrc = "C:\TridjsStems\htdemucs_compilado.pt"
if (-not (Test-Path $modelSrc)) {
    $modelSrc = Join-Path $EXE_DIR "htdemucs_compilado.pt"
}
if (Test-Path $modelSrc) {
    Copy-Item $modelSrc "$Output\resources\"
    Write-Host "[OK] Modelo IA: htdemucs_compilado.pt" -ForegroundColor Green
} else {
    Write-Host "[AVISO] Modelo IA nao encontrado - adicione manualmente." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "--- GRUPO A: DLLs CPU (obrigatorias em TODOS os clientes) ---" -ForegroundColor White

# 4. DLLs CPU - obrigatorias
$CPU_DLLS = @(
    "c10.dll",
    "torch.dll",
    "torch_cpu.dll",
    "fbgemm.dll",
    "libiomp5md.dll",
    "libiompstubs5md.dll",
    "mkl_core.1.dll",
    "mkl_intel_thread.1.dll",
    "asmjit.dll",
    "uv.dll",
    "zlibwapi.dll",
    "pytorch_jni.dll",
    "fbjni.dll",
    "torch_global_deps.dll"
)

$cpuMissing = 0
foreach ($dll in $CPU_DLLS) {
    $src = Join-Path $LIBTORCH_DIR $dll
    # Tenta tambem pegar da pasta do exe (ja copiadas anteriormente)
    if (-not (Test-Path $src)) { $src = Join-Path $EXE_DIR $dll }
    if (Test-Path $src) {
        Copy-Item $src $Output -Force
        Write-Host "  [CPU-OK ] $dll" -ForegroundColor Green
    } else {
        Write-Host "  [CPU-ERR] $dll NAO ENCONTRADA - build pode falhar no cliente!" -ForegroundColor Red
        $cpuMissing++
    }
}

Write-Host ""
Write-Host "--- GRUPO B: DLLs GPU/CUDA (opcionais - aceleracao NVIDIA) ---" -ForegroundColor White
Write-Host "    (PCs sem GPU NVIDIA funcionam normalmente com CPU)" -ForegroundColor DarkGray

# 5. DLLs GPU - opcionais
$GPU_DLLS = @(
    "torch_cuda.dll",
    "c10_cuda.dll",
    "caffe2_nvrtc.dll",
    "cudart64_12.dll",
    "cublas64_12.dll",
    "cublasLt64_12.dll",
    "cudnn64_8.dll",
    "cudnn_adv_infer64_8.dll",
    "cudnn_adv_train64_8.dll",
    "cudnn_cnn_infer64_8.dll",
    "cudnn_cnn_train64_8.dll",
    "cudnn_ops_infer64_8.dll",
    "cudnn_ops_train64_8.dll",
    "cufft64_11.dll",
    "cufftw64_11.dll",
    "curand64_10.dll",
    "cusolver64_11.dll",
    "cusolverMg64_11.dll",
    "cusparse64_12.dll",
    "nvrtc64_120_0.dll",
    "nvrtc-builtins64_121.dll",
    "nvJitLink_120_0.dll",
    "nvToolsExt64_1.dll",
    "cupti64_2023.1.1.dll"
)

$gpuFound = 0
foreach ($dll in $GPU_DLLS) {
    $src = Join-Path $LIBTORCH_DIR $dll
    if (-not (Test-Path $src)) { $src = Join-Path $EXE_DIR $dll }
    if (Test-Path $src) {
        Copy-Item $src $Output -Force
        Write-Host "  [GPU-OK ] $dll" -ForegroundColor Cyan
        $gpuFound++
    } else {
        Write-Host "  [GPU-N/A] $dll (nao encontrada - cliente sem GPU usara CPU)" -ForegroundColor DarkGray
    }
}

# 6. Cria README de distribuicao
$readme = @"
TriDJs Stems - Pacote de Distribuicao
======================================
Versao : 1.0.0
Config : $Config
Data   : $(Get-Date -Format "yyyy-MM-dd HH:mm")

REQUISITOS MINIMOS
------------------
- Windows 10 64-bit ou superior
- 8 GB de RAM (16 GB recomendado)
- 4 GB de espaco em disco

COM GPU NVIDIA (recomendado)
- NVIDIA GPU com CUDA Compute Capability 6.0+ (GTX 1060 ou superior)
- Processamento ~5x mais rapido

SEM GPU (CPU-only)
- Funciona em qualquer PC Windows 64-bit
- Processamento mais lento (1-3 minutos por musica)

INSTALACAO
----------
1. Extraia todos os arquivos na mesma pasta
2. Execute "TriDJs Stems.exe"
3. O programa detecta automaticamente se ha GPU disponivel

NAO MOVA o .exe para outra pasta sem mover as DLLs junto!

SUPORTE: https://github.com/webeder/Tridjs-Stems-Suite
"@
$readme | Out-File -FilePath "$Output\LEIA-ME.txt" -Encoding UTF8

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Empacotamento concluido!" -ForegroundColor Green
Write-Host "  Pasta de saida  : $Output" -ForegroundColor White
Write-Host "  DLLs CPU        : $($CPU_DLLS.Count - $cpuMissing) / $($CPU_DLLS.Count)" -ForegroundColor White
Write-Host "  DLLs GPU        : $gpuFound / $($GPU_DLLS.Count)" -ForegroundColor White
if ($cpuMissing -gt 0) {
    Write-Host "  [AVISO] $cpuMissing DLL(s) CPU faltando - verifique o libtorch!" -ForegroundColor Red
}
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
