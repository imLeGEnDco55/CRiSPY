#include "PluginProcessor.h"
#include "PluginEditor.h"

#ifdef _WIN32
#include <windows.h>
// Get the HMODULE of this DLL (not the host DAW)
static HMODULE getThisDllModule()
{
    HMODULE hModule = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)&getThisDllModule,
        &hModule);
    return hModule;
}

static juce::File getThisDllPath()
{
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(getThisDllModule(), path, MAX_PATH);
    return juce::File(juce::String(path));
}
#endif

//==============================================================================
CrispyProcessor::CrispyProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    loadApiKey();
    if (apiKey.isNotEmpty())
        appState.store(CRISPY_READY);
}

CrispyProcessor::~CrispyProcessor() {}

//==============================================================================
void CrispyProcessor::prepareToPlay(double, int) {}
void CrispyProcessor::releaseResources() {}

bool CrispyProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void CrispyProcessor::processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&)
{
    // Pure pass-through — CRiSPY does file-based processing, not real-time
}

//==============================================================================
juce::AudioProcessorEditor* CrispyProcessor::createEditor()
{
    return new CrispyEditor(*this);
}

//==============================================================================
void CrispyProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree state("CRiSPY");
    state.setProperty("apiKey", apiKey, nullptr);
    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void CrispyProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, (size_t)sizeInBytes);
    if (tree.isValid())
    {
        apiKey = tree.getProperty("apiKey", "").toString();
        if (apiKey.isNotEmpty())
            appState.store(CRISPY_READY);
    }
}

//==============================================================================
// Config file persistence (AppData)
juce::File CrispyProcessor::getConfigFile() const
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("CRiSPY");
    appDataDir.createDirectory();
    return appDataDir.getChildFile("config.txt");
}

void CrispyProcessor::saveApiKey()
{
    getConfigFile().replaceWithText(apiKey);
}

void CrispyProcessor::loadApiKey()
{
    auto configFile = getConfigFile();
    if (configFile.existsAsFile())
    {
        apiKey = configFile.loadFileAsString().trim();
    }
}

void CrispyProcessor::setApiKey(const juce::String& key)
{
    apiKey = key;
    saveApiKey();
    appState.store(CRISPY_READY);
}

void CrispyProcessor::resetToReady()
{
    cleanFilePath.clear();
    errorMessage.clear();
    appState.store(CRISPY_READY);
}

//==============================================================================
// Locate python.exe
juce::File CrispyProcessor::locatePythonExecutable() const
{
    // Check standard Python install paths on Windows
    auto localApp = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getParentDirectory().getChildFile("Local");
    for (int v = 15; v >= 8; --v)
    {
        auto testPath = localApp.getChildFile("Programs")
                                .getChildFile("Python")
                                .getChildFile("Python3" + juce::String(v))
                                .getChildFile("python.exe");
        if (testPath.existsAsFile())
            return testPath;
    }

    // Check Program Files
    juce::StringArray progDirs = { "C:\\Python312", "C:\\Python311", "C:\\Python310", "C:\\Python39" };
    for (auto& d : progDirs)
    {
        auto f = juce::File(d).getChildFile("python.exe");
        if (f.existsAsFile())
            return f;
    }

    // Fallback: use PATH (ChildProcess will resolve it)
    return juce::File("python");
}

// Locate process_audio.py — use the DLL's actual location
juce::File CrispyProcessor::locatePythonScript() const
{
#ifdef _WIN32
    auto dllFile = getThisDllPath();
    auto dllDir = dllFile.getParentDirectory();  // x86_64-win/

    // VST3 bundle structure: CRiSPY.vst3/Contents/x86_64-win/CRiSPY.vst3
    //                        CRiSPY.vst3/Contents/python/process_audio.py
    auto contentsDir = dllDir.getParentDirectory(); // Contents/
    auto bundleDir = contentsDir.getParentDirectory(); // CRiSPY.vst3/

    juce::Array<juce::File> candidates = {
        contentsDir.getChildFile("python").getChildFile("process_audio.py"),
        bundleDir.getChildFile("python").getChildFile("process_audio.py"),
        dllDir.getChildFile("python").getChildFile("process_audio.py"),
    };

    for (auto& p : candidates)
    {
        if (p.existsAsFile())
            return p;
    }
#endif

    // Hardcoded fallback for development
    auto devPath = juce::File("c:\\Apps\\CRiSPY\\python\\process_audio.py");
    if (devPath.existsAsFile())
        return devPath;

    return juce::File();
}

//==============================================================================
// Launch Python processing in a background thread
void CrispyProcessor::launchProcessing(const juce::String& inputFilePath)
{
    appState.store(CRISPY_PROCESSING);

    // Build output path in temp directory
    auto inputFile = juce::File(inputFilePath);
    auto outputFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("enhanced_" + inputFile.getFileName());
    cleanFilePath = outputFile.getFullPathName();

    // Delete existing output
    outputFile.deleteFile();

    auto pythonExe = locatePythonExecutable();
    auto scriptPath = locatePythonScript();
    auto key = apiKey;
    auto outPath = cleanFilePath;

    // Validate script path first
    if (!scriptPath.existsAsFile())
    {
        errorMessage = "Cannot find process_audio.py. Path: " + scriptPath.getFullPathName();
        appState.store(CRISPY_ERROR);
        return;
    }

    // Launch background thread
    std::thread([this, pythonExe, scriptPath, inputFilePath, outPath, key]()
    {
        juce::String cmdLine = pythonExe.getFullPathName().quoted()
                             + " " + scriptPath.getFullPathName().quoted()
                             + " --input " + inputFilePath.quoted()
                             + " --output " + outPath.quoted()
                             + " --api-key " + key.quoted();

        juce::ChildProcess process;
        bool started = process.start(cmdLine);

        if (started)
        {
            // Read all output (stdout + stderr) while process runs
            juce::String processOutput;
            while (process.isRunning())
            {
                auto chunk = process.readAllProcessOutput();
                if (chunk.isNotEmpty())
                    processOutput += chunk;
                juce::Thread::sleep(100);
            }
            // Read remaining output after process finishes
            processOutput += process.readAllProcessOutput();

            auto exitCode = process.getExitCode();

            if (exitCode == 0)
            {
                appState.store(CRISPY_COMPLETE);
            }
            else
            {
                // Show actual Python error output
                if (processOutput.isNotEmpty())
                    errorMessage = "Exit " + juce::String(exitCode) + ": " + processOutput.substring(0, 500);
                else
                    errorMessage = "API error: exit code " + juce::String(exitCode) + ". Verify your Nvidia key and internet.";
                appState.store(CRISPY_ERROR);
            }
        }
        else
        {
            errorMessage = "Failed to run: " + pythonExe.getFullPathName() + ". Is Python installed?";
            appState.store(CRISPY_ERROR);
        }
    }).detach();
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrispyProcessor();
}
