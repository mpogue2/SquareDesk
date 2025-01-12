#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED = 1
#define DEBUG 1
#define JUCE_PLUGINHOST_AU 1
#define JUCE_MAC 1

#include "JuceHeader.h"

// NOTE: the JUCE static library is built using ProJucer, and
// it must include at least the following:
//
// audio_basics
// audio_formats
// audio_processors
// core
// data_structures
// dsp
// events
// graphics [MIGHT be optional, haven't tried yet]
// gui_basics [MIGHT be optional, haven't tried yet]
// gui_extras [MIGHT be optional, haven't tried yet]

#include <QString>
#include <QDebug>
#include <memory>

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include <QDebug>

using namespace juce;

#ifdef OLDSCAN
class PluginFinder : public juce::FileFilter
{
public:
    // Update file filter for macOS VST/AU formats
    // VSTScanner() : FileFilter("*.vst;*.vst3;*.component;*.audiounit") {}
    PluginFinder() : FileFilter("*.audiounit") {}

    void scanForPlugins(KnownPluginList &kpl)
    {
        // std::vector<PluginInfo> plugins;

        juce::MessageManager::getInstance();

        // Create plugin format manager
        auto formatManager = std::make_unique<juce::AudioPluginFormatManager>();
        formatManager->addDefaultFormats();

        for (auto e : formatManager->getFormats()) {
            qDebug() << "FormatManager understands:" << e->getName().toStdString();
        }

        // KnownPluginList kpl;
        AudioUnitPluginFormat aupf;

        PluginDirectoryScanner scanner(kpl, aupf, aupf.getDefaultLocationsToSearch(), true, File());
        String plugname;
        while (true)
        {
            String nextname = scanner.getNextPluginFileThatWillBeScanned();
            // qDebug() << nextname.toStdString();
            if (scanner.scanNextFile(true, plugname) == false) {
                break;
            }
        }
    }

    bool isFileSuitable(const juce::File& file) const override
    {
        return /*file.hasFileExtension("vst") ||
               file.hasFileExtension("vst3") ||
               file.hasFileExtension("component") ||*/
               file.hasFileExtension("audiounit");
    }

    bool isDirectorySuitable(const juce::File& file) const override
    {
        (void)file;
        return false;
    }
};
#endif

// --------------------------------------------
void MainWindow::scanForPlugins() {
    qDebug() << "PLUGINS =============";

#ifdef OLDSCAN
    PluginFinder scanner;

    // Scan for plugins
    KnownPluginList kpl;
    scanner.scanForPlugins(kpl);

    Array<PluginDescription> types = kpl.getTypes();

    PluginDescription loudMaxPluginDescription;
    for (auto element : types) {
        qDebug() << element.pluginFormatName.toStdString() << element.manufacturerName.toStdString() << element.name.toStdString();
        if (element.name.toStdString() == "LoudMax") {
            qDebug() << "Found the LoudMax:" << element.pluginFormatName.toStdString() << element.fileOrIdentifier.toStdString();
            loudMaxPluginDescription = element;
            break;
        }
    }
#else
    // ALTERNATE: does this work?  Why yes, it does. This is way simpler, if we are only loading
    //    1 plugin from a known location.
    //    https://forum.juce.com/t/instantiate-plug-in/34099/2
    PluginDescription altDesc;
    File home = File::getSpecialLocation(File::SpecialLocationType::userHomeDirectory);
    altDesc.fileOrIdentifier = home.getChildFile("Library/Audio/Plug-Ins/Components/LoudMax.component").getFullPathName();
    altDesc.pluginFormatName = "AudioUnit";
#endif
    // HOW TO INSTANTIATE A PLUGIN
    // https://forum.juce.com/t/instantiate-plug-in/34099/2

    String error;
    AudioPluginFormatManager plugmgr;
    plugmgr.addDefaultFormats();

    // std::unique_ptr<AudioPluginInstance> loudMaxPlugin =
#ifdef OLDSCAN
    loudMaxPlugin =
        plugmgr.createPluginInstance(loudMaxPluginDescription, 44100.0, 512, error); // THIS WORKS
#else
    loudMaxPlugin =
        plugmgr.createPluginInstance(altDesc, 44100.0, 512, error);  // THIS WORKS (and we could ship with an embedded copy of LoudMax?)
#endif

    if (loudMaxPlugin == nullptr) {
        qDebug() << error.toStdString(); // there was an error in loading
        return;
    } else {
        qDebug() << "LoudMax was loaded!";
    }

    // loudMaxPlugin->createEditorIfNeeded();

    qDebug() << "*** LoudMax:" << loudMaxPlugin->getSampleRate()
             << loudMaxPlugin->getLatencySamples()
             << loudMaxPlugin->getTotalNumInputChannels()
             << loudMaxPlugin->getTotalNumOutputChannels();

    // Array< AudioProcessorParameter * > parms = loudMaxPlugin->getParameters();
    // for (auto a : parms) {
    //     qDebug() << "LoudMax parameter:"
    //              << a->getName(15).toStdString()
    //              << a->getCurrentValueAsText().toStdString();
    //     for (auto b : a->getAllValueStrings()) {
    //         qDebug() << "   possible value:" << b.toStdString();
    //     }
    // }

// LoudMax parameter: "Thresh" "0.0"
// LoudMax parameter: "Output" "0.0"
// LoudMax parameter: "Fader Link" "Off"
//     possible value: "Off"
//     possible value: "On"
// LoudMax parameter: "ISP Detection" "Off"
//     possible value: "Off"
//     possible value: "On"
// LoudMax parameter: "Large GUI" "Off"
//     possible value: "Off"
//     possible value: "On"

    auto paramThresh = loudMaxPlugin->getHostedParameter(0); // 0.0 - 1.0 maps to -30 - 0 dB threshold. This is the important one.
    // auto paramOutput = loudMaxPlugin->getHostedParameter(1); // OK to leave at 1.0 = 0.0dB output
    // auto paramFaderLink = loudMaxPlugin->getHostedParameter(2); // leave at OFF. We do not need to link the Thresh and Output faders.
    // auto paramISPDetect = loudMaxPlugin->getHostedParameter(3); // leave at OFF. We do not need to detect peaks BETWEEN samples (takes lots of CPU).
    // auto paramLargeGUI = loudMaxPlugin->getHostedParameter(4);  // leave at OFF. We do not use the GUI right now.

    qDebug() << "Before: " << paramThresh->getValue()
             << paramThresh->getCurrentValueAsText().toStdString()
             << paramThresh->getDefaultValue();
    paramThresh->setValue(0.5);
    qDebug() << "After: " << paramThresh->getValue()
             << paramThresh->getCurrentValueAsText().toStdString()
             << paramThresh->getDefaultValue();

    // GOOD: https://forum.juce.com/t/hosting-plugins/44560/13
    // You use the PluginDescription that your scanning returned to create an instance of the plugin.
    //     Then you need to prepare the AudioProcessor by calling prepareToPlay() and then
    //     you can process the audio using the processBlock() method.

    qDebug() << "ABOUT TO OPEN A PLUGIN WINDOW =============";

    // https://forum.juce.com/t/open-a-window-in-juce-and-put-a-plugineditor-into-it/54102/5
    // // loudMaxWin = std::make_unique<juce::ResizableWindow>(String("LoudMax"), true);

    // This works, but it leaks.  I think we should just have a single slider in SquareDesk.
    //   Probably per-song?

    // loudMaxWin = new juce::DocumentWindow(String("LoudMax"), juce::Colours::white, juce::DocumentWindow::closeButton, true);
    // loudMaxWin->setUsingNativeTitleBar(true);
    // loudMaxWin->setContentOwned (loudMaxPlugin->createEditor(), true);
    // loudMaxWin->addToDesktop (/* flags */);
    // loudMaxWin->setVisible (true);

    qDebug() << "PLUGINS =============";
}
