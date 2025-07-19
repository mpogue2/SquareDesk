#include "globaldefines.h"

// Disable warning, see: https://github.com/llvm/llvm-project/issues/48757
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Welaborated-enum-base"
#include "mainwindow.h"
#pragma clang diagnostic pop

#include "ui_mainwindow.h"

#ifdef USE_JUCE

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
#include <QWindow>
#include <QEvent>
#include <QGuiApplication>
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


class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow(const juce::String& name,
                 juce::Colour backgroundColour,
                 int requiredButtons,
                 bool addToDesktop,
                 Ui::MainWindow *ui1)
        : DocumentWindow(name, backgroundColour, requiredButtons, addToDesktop)
    {
        setUsingNativeTitleBar(true);
        ui = ui1;
    }

    void closeButtonPressed() override
    {
        // Just hide the window instead of destroying it
        setVisible(false);
        ui->FXbutton->setChecked(false);
    }

#ifdef USE_JUCE
    // Function to sync window level with the main window
    void syncWindowLevelWithMain(QWindow* mainWindow)
    {
#ifdef Q_OS_MAC
        // On macOS, use native APIs to sync window levels
        if (mainWindow && isVisible()) {
            syncWindowLevelsNative(mainWindow->winId(), getWindowHandle());
        }
#endif
    }
#endif

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
    Ui::MainWindow *ui;
    
#ifdef Q_OS_MAC
    // Platform-specific implementation for macOS
    static void syncWindowLevelsNative(WId mainWindowId, void* pluginWindowHandle);
#endif
};

// --------------------------------------------
void MainWindow::scanForPlugins() {
    // qDebug() << "PLUGINS =============";

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
    // Try VST2 version first (often more compatible), then VST3, then AudioUnit
    PluginDescription vstDesc;
    File home = File::getSpecialLocation(File::SpecialLocationType::userHomeDirectory);
    
    // // First try VST2 version (more compatible with older JUCE versions)
    // vstDesc.fileOrIdentifier = home.getChildFile("Library/Audio/Plug-Ins/VST/LoudMax.vst").getFullPathName();
    // vstDesc.pluginFormatName = "VST";
    
    // qDebug() << "Trying VST2 version at:" << vstDesc.fileOrIdentifier.toStdString().c_str();
    
    // // Check if VST2 file exists
    // File vstFile(vstDesc.fileOrIdentifier);
    // if (!vstFile.exists()) {
    //     qDebug() << "VST2 not found, trying VST3...";
    //     // Try VST3 version
    //     vstDesc.fileOrIdentifier = home.getChildFile("Library/Audio/Plug-Ins/VST3/LoudMax.vst3").getFullPathName();
    //     vstDesc.pluginFormatName = "VST3";
        
    //     vstFile = File(vstDesc.fileOrIdentifier);
    //     if (!vstFile.exists()) {
    //         qDebug() << "VST3 not found, falling back to AudioUnit...";
    //         // Fallback to AudioUnit
    //         vstDesc.fileOrIdentifier = home.getChildFile("Library/Audio/Plug-Ins/Components/LoudMax.component").getFullPathName();
    //         vstDesc.pluginFormatName = "AudioUnit";
    //     }
    // }

    // Try VST3 version
    vstDesc.fileOrIdentifier = home.getChildFile("Library/Audio/Plug-Ins/VST3/LoudMax.vst3").getFullPathName();
    vstDesc.pluginFormatName = "VST3";

    File vstFile(vstDesc.fileOrIdentifier);
    // vstFile = File(vstDesc.fileOrIdentifier);
    if (!vstFile.exists()) {
        // qDebug() << "VST3 not found, falling back to AudioUnit...";
        // Fallback to AudioUnit
        vstDesc.fileOrIdentifier = home.getChildFile("Library/Audio/Plug-Ins/Components/LoudMax.component").getFullPathName();
        vstDesc.pluginFormatName = "AudioUnit";
    }

    // qDebug() << "Using plugin format:" << vstDesc.pluginFormatName.toStdString().c_str();
    // qDebug() << "Plugin path:" << vstDesc.fileOrIdentifier.toStdString().c_str();
#endif
    // HOW TO INSTANTIATE A PLUGIN
    // https://forum.juce.com/t/instantiate-plug-in/34099/2

    String errorMessage;
    AudioPluginFormatManager plugmgr;
    plugmgr.addDefaultFormats();

#ifdef OLDSCAN
    loudMaxPlugin =
        plugmgr.createPluginInstance(loudMaxPluginDescription, 44100.0, 512, error); // THIS WORKS
#else
    // For the direct file approach, we need to scan the plugin file first
    // qDebug() << "Scanning plugin file to get proper description...";
    // qDebug() << "Available plugin formats:";
    // for (auto* format : plugmgr.getFormats()) {
    //     qDebug() << "  -" << format->getName().toStdString().c_str();
    // }
    
    PluginDescription foundDesc;
    bool pluginFound = false;
    
    File pluginFile(vstDesc.fileOrIdentifier);
    if (pluginFile.exists()) {
        // qDebug() << "Plugin file exists, attempting to scan...";
        
        // Get the appropriate format for scanning
        for (auto* format : plugmgr.getFormats()) {
            // qDebug() << "Checking format:" << format->getName().toStdString().c_str() << "vs" << vstDesc.pluginFormatName.toStdString().c_str();
            if (format->getName() == vstDesc.pluginFormatName) {
                // qDebug() << "Found format manager for:" << format->getName().toStdString().c_str();
                
                try {
                    OwnedArray<PluginDescription> descriptions;
                    format->findAllTypesForFile(descriptions, vstDesc.fileOrIdentifier);
                    
                    // qDebug() << "Found" << descriptions.size() << "plugin(s) in file";
                    
                    if (descriptions.size() > 0) {
                        foundDesc = *descriptions[0]; // Use first plugin found
                        pluginFound = true;
                        // qDebug() << "Using plugin:" << foundDesc.name.toStdString().c_str()
                        //          << format->getName().toStdString().c_str()
                        //          << vstDesc.fileOrIdentifier.toStdString().c_str();
                        QString theToolTip = QString("LoudMax: ") + format->getName().toStdString().c_str() + " Compressor\n" +
                            "Per-song compression settings ENABLED";
                        // if (QString(format->getName().toStdString().c_str()) == "AudioUnit") {
                        //     theToolTip += " DISABLED";
                        // } else {
                        //     theToolTip += " ENABLED";
                        // }
                        ui->FXbutton->setToolTip(theToolTip);
                        break;
                    } else {
                        qDebug() << "No plugins found in file by" << format->getName().toStdString().c_str();
                    }
                } catch (const std::exception& e) {
                    qDebug() << "Exception during plugin scan:" << e.what();
                } catch (...) {
                    qDebug() << "Unknown exception during plugin scan";
                }
            }
        }
    }
    
    if (pluginFound) {
        loudMaxPlugin = plugmgr.createPluginInstance(foundDesc, 44100.0, 512, errorMessage);
        if (loudMaxPlugin == nullptr) {
            qDebug() << "ERROR 101: " << QString::fromStdString(errorMessage.toStdString());
        }
    } else {
        // qDebug() << "Could not scan plugin file, falling back to AudioUnit...";
        // Fallback to AudioUnit with proper scanning
        File auFile = File::getSpecialLocation(File::SpecialLocationType::userHomeDirectory)
                        .getChildFile("Library/Audio/Plug-Ins/Components/LoudMax.component");
        
        if (auFile.exists()) {
            for (auto* format : plugmgr.getFormats()) {
                if (format->getName() == "AudioUnit") {
                    OwnedArray<PluginDescription> descriptions;
                    format->findAllTypesForFile(descriptions, auFile.getFullPathName());
                    
                    if (descriptions.size() > 0) {
                        loudMaxPlugin = plugmgr.createPluginInstance(*descriptions[0], 44100.0, 512, errorMessage);
                        if (loudMaxPlugin == nullptr) {
                            qDebug() << "ERROR 102: " << QString::fromStdString(errorMessage.toStdString());
                        }
                        // qDebug() << "Loaded AudioUnit fallback successfully";
                        ui->FXbutton->setToolTip("LoudMax: AU Compressor\nPer-song compression settings ENABLED");
                        break;
                    }
                }
            }
        } else {
            // qDebug() << "LoudMax AU file does not exist.";
        }
    }
#endif

    if (loudMaxPlugin == nullptr) {
        // qDebug() << "ERROR 103: " << errorMessage.toStdString(); // there was an error in loading
        ui->FXbutton->setVisible(false);
        return;
    } else {
        // qDebug() << "LoudMax was loaded!";
    }

    ui->FXbutton->setVisible(true);
    
    // Initialize LED state
    currentFXButtonLEDState = false; // Start with unknown state to force update
    updateFXButtonLED(true);         // Tricky: set to true, so that the set to false will register as a change
    updateFXButtonLED(false);        // Initialize to grey LED

    // loudMaxPlugin->createEditorIfNeeded();

    // qDebug() << "*** LoudMax:" << loudMaxPlugin->getSampleRate()
    //          << loudMaxPlugin->getLatencySamples()
    //          << loudMaxPlugin->getTotalNumInputChannels()
    //          << loudMaxPlugin->getTotalNumOutputChannels()
    //          << loudMaxPlugin->isUsingDoublePrecision()
    //          << loudMaxPlugin->getTailLengthSeconds();

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

    // paramThresh = loudMaxPlugin->getHostedParameter(0); // 0.0 - 1.0 maps to -30 - 0 dB threshold. This is the important one.
    // // auto paramOutput = loudMaxPlugin->getHostedParameter(1); // OK to leave at 1.0 = 0.0dB output
    // // auto paramFaderLink = loudMaxPlugin->getHostedParameter(2); // leave at OFF. We do not need to link the Thresh and Output faders.
    // // auto paramISPDetect = loudMaxPlugin->getHostedParameter(3); // leave at OFF. We do not need to detect peaks BETWEEN samples (takes lots of CPU).
    // // auto paramLargeGUI = loudMaxPlugin->getHostedParameter(4);  // leave at OFF. We do not use the GUI right now.

    // qDebug() << "Before: " << paramThresh->getValue()
    //          << paramThresh->getCurrentValueAsText().toStdString()
    //          << paramThresh->getDefaultValue();
    // paramThresh->setValue(0.5);
    // qDebug() << "After: " << paramThresh->getValue()
    //          << paramThresh->getCurrentValueAsText().toStdString()
    //          << paramThresh->getDefaultValue();

    // GOOD: https://forum.juce.com/t/hosting-plugins/44560/13
    // You use the PluginDescription that your scanning returned to create an instance of the plugin.
    //     Then you need to prepare the AudioProcessor by calling prepareToPlay() and then
    //     you can process the audio using the processBlock() method.

    // qDebug() << "ABOUT TO OPEN A PLUGIN WINDOW =============";

    // https://forum.juce.com/t/open-a-window-in-juce-and-put-a-plugineditor-into-it/54102/5
    // // loudMaxWin = std::make_unique<juce::ResizableWindow>(String("LoudMax"), true);

    // This works, but it leaks.  I think we should just have a single slider in SquareDesk.
    //   Probably per-song?

    // loudMaxWin = new PluginWindow(String("FX: LoudMax"), juce::Colours::white, juce::DocumentWindow::closeButton, true);
    loudMaxWin = std::make_unique<PluginWindow>(String("Global FX: LoudMax"),
                                                juce::Colours::white,
                                                juce::DocumentWindow::closeButton,
                                                true,
                                                ui);
    loudMaxWin->setUsingNativeTitleBar(true);
    // Don't set to always on top - we want it to follow the same layering as the main window
    // loudMaxWin->setAlwaysOnTop(true);
    loudMaxWin->setContentOwned (loudMaxPlugin->createEditor(), true);
    // loudMaxWin->setContentOwned (new GenericAudioProcessorEditor(*loudMaxPlugin), true);
    loudMaxWin->addToDesktop (/* flags */);

    connect(ui->FXbutton, &QPushButton::clicked,
            this, [this](){
                if (ui->FXbutton->isChecked()) {
                    QPoint p = ui->FXbutton->mapToGlobal(QPoint(0,0));
                    loudMaxWin->setBounds(p.x() /*+ loudMaxWin->getWidth() + ui->FXbutton->width()*/,
                                          p.y() + ui->FXbutton->height() + 30,
                                          loudMaxWin->getWidth(),
                                          loudMaxWin->getHeight());
                    loudMaxWin->setVisible (true); // show the window!
                    
                    // Sync window levels when showing
                    static_cast<PluginWindow*>(loudMaxWin.get())->syncWindowLevelWithMain(windowHandle());
                } else {
                    loudMaxWin->setVisible (false); // hide the window!
                }
            } );

    // Install event filter on the main window to catch window state changes
    this->installEventFilter(this);
    
    // Also connect to the application state changes for better window level management
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && loudMaxWin && loudMaxWin->isVisible()) {
            // When application becomes active again, make sure window levels are in sync
            static_cast<PluginWindow*>(loudMaxWin.get())->syncWindowLevelWithMain(windowHandle());
            
            // On macOS, explicitly raise the LoudMax window to ensure it comes to the front
#ifdef Q_OS_MAC
            loudMaxWin->toFront(false); // false means don't become the key window
#endif
        }
    });
    
    extern flexible_audio *cBass;
    cBass->setLoudMaxPlugin(loudMaxPlugin); // cBass is a flexible_audio, which passes to AudioDecoder, which passes to PlayerThread
                                            //   and PlayerThread calls PrepareToPlay and ProcessBlock to process audio.

    // qDebug() << "PLUGINS =============";
}

// Handle window state changes
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
#ifdef USE_JUCE
    if (watched == this && event->type() == QEvent::WindowStateChange) {
        // Main window state changed (minimized or restored)
        bool isMinimized = windowHandle()->windowState() & Qt::WindowMinimized;
        
        // Store whether LoudMax was visible before minimizing
        static bool wasLoudMaxVisible = false;
        
        if (isMinimized) {
            // Main window is being minimized
            if (loudMaxWin) {
                // Remember if LoudMax was visible
                wasLoudMaxVisible = loudMaxWin->isVisible();
                
                // Hide LoudMax when main window is minimized
                if (wasLoudMaxVisible) {
                    loudMaxWin->setVisible(false);
                }
            }
        } else {
            // Main window is being restored
            if (loudMaxWin && wasLoudMaxVisible && ui->FXbutton->isChecked()) {
                // Restore LoudMax if it was visible before and button is still checked
                loudMaxWin->setVisible(true);
                
                // Sync window levels after restoring
                static_cast<PluginWindow*>(loudMaxWin.get())->syncWindowLevelWithMain(windowHandle());
            }
        }
    }
    else if (watched == this && event->type() == QEvent::ActivationChange) {
        // Main window activation changed
        if (isActiveWindow() && loudMaxWin && loudMaxWin->isVisible()) {
            // Sync window levels when activated
            static_cast<PluginWindow*>(loudMaxWin.get())->syncWindowLevelWithMain(windowHandle());
            
            // Also explicitly raise the LoudMax window when main window is activated
#ifdef Q_OS_MAC
            loudMaxWin->toFront(false); // false means don't become the key window
#endif
        }
    }
#endif

    // Call the base class implementation
    return QMainWindow::eventFilter(watched, event);
}

#endif

// ==== Platform-specific implementation ====
#ifdef Q_OS_MAC
#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif

// Implement the static method defined in PluginWindow
void PluginWindow::syncWindowLevelsNative(WId mainWindowId, void* pluginWindowHandle)
{
#ifdef __OBJC__
    // Implementation using Objective-C
    NSView* mainView = reinterpret_cast<NSView*>(mainWindowId);
    NSWindow* mainNSWindow = [mainView window];
    
    NSView* pluginView = (NSView*)pluginWindowHandle;
    NSWindow* pluginNSWindow = [pluginView window];
    
    if (mainNSWindow && pluginNSWindow) {
        // Sync the window level - ensure LoudMax has exactly the same level as the main window
        [pluginNSWindow setLevel:[mainNSWindow level]];
        
        // Sync collection behavior - this controls how the window behaves with macOS window management
        NSWindowCollectionBehavior behavior = [mainNSWindow collectionBehavior];
        [pluginNSWindow setCollectionBehavior:behavior];
        
        // Ensure floating behavior matches
        [pluginNSWindow setFloatingPanel:[mainNSWindow isFloatingPanel]];
        
        // Make sure the plugin window moves with the main window
        [pluginNSWindow setMovableByWindowBackground:YES];
        
        // Force order the plugin window above the main window
        if ([mainNSWindow isVisible] && [pluginNSWindow isVisible]) {
            [pluginNSWindow orderWindow:NSWindowAbove relativeTo:[mainNSWindow windowNumber]];
        }
    }
#else
    // Silence unused parameter warnings when not using Objective-C
    (void)mainWindowId;
    (void)pluginWindowHandle;
#endif
}
#endif // Q_OS_MAC

// =============================================================================
// PER-SONG PERSISTANCE OF LOUDMAX PARAMS

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

// // auto paramThresh = loudMaxPlugin->getHostedParameter(0); // 0.0 - 1.0 maps to -30 - 0 dB threshold. This is the important one.
// // auto paramOutput = loudMaxPlugin->getHostedParameter(1); // OK to leave at 1.0 = 0.0dB output
// // auto paramFaderLink = loudMaxPlugin->getHostedParameter(2); // leave at OFF. We do not need to link the Thresh and Output faders.
// // auto paramISPDetect = loudMaxPlugin->getHostedParameter(3); // leave at OFF. We do not need to detect peaks BETWEEN samples (takes lots of CPU).
// // auto paramLargeGUI = loudMaxPlugin->getHostedParameter(4);  // leave at OFF. We do not use the GUI right now.

// qDebug() << "Before: " << paramThresh->getValue()
//          << paramThresh->getCurrentValueAsText().toStdString()
//          << paramThresh->getDefaultValue();
// paramThresh->setValue(0.5);
// qDebug() << "After: " << paramThresh->getValue()
//          << paramThresh->getCurrentValueAsText().toStdString()
//          << paramThresh->getDefaultValue();

QString MainWindow::getCurrentLoudMaxSettings() {

    if (loudMaxPlugin == nullptr) {
        return("");  // no LoudMax, so return empty string nothing to do
    }

    QStringList persistParameters;
    Array< AudioProcessorParameter * > parms = loudMaxPlugin->getParameters();
    for (auto a : parms) {
        // qDebug() << "getCurrentLoudMaxSettings: LoudMax parameter:"
        //          << a->getName(15).toStdString()
        //          << a->getCurrentValueAsText().toStdString()
        //          << a->getValue();
        persistParameters.append(QString::fromStdString(a->getName(15).toStdString()) + "=" + QString::number(a->getValue()));
        // for (auto b : a->getAllValueStrings()) {
        //     qDebug() << "   possible value:" << b.toStdString();
        // }
    }

    QString persistParameterString = QString("LoudMax:") + persistParameters.join(",");
    // qDebug() << "getCurrentLoudMaxSettings:" << persistParameterString;

    return(persistParameterString);  // e.g. "LoudMax:Thresh=0.9,Output=1.0, ... "
}

void MainWindow::setLoudMaxFromPersistedSettings(QString persistParameterString) {
    // qDebug() << "setLoudMaxFromPersistedSettings:" << persistParameterString;

    if (loudMaxPlugin == nullptr) {
        return; // no LoudMax plugin loaded
    }

    // Parse the settings string to extract parameter values
    if (!persistParameterString.startsWith("LoudMax:")) {
        return; // not a LoudMax settings string
    }

    QString paramString = persistParameterString.mid(8); // remove "LoudMax:" prefix
    QStringList params = paramString.split(",");
    
    // Parse all parameters
    QMap<QString, float> paramValues;
    for (const QString& param : params) {
        QStringList keyVal = param.split("=");
        if (keyVal.size() == 2) {
            QString key = keyVal[0].trimmed();
            float value = keyVal[1].toFloat();
            paramValues[key] = value;
            
            // // Convert threshold to dB for logging
            // if (key == "Thresh") {
            //     float thresholdDB = -30.0f + (value * 30.0f); // 0.0→-30dB, 1.0→0dB
            //     qDebug() << "Setting LoudMax" << key << "to" << thresholdDB << "dB (parameter value:" << value << ")";
            // } else {
            //     qDebug() << "Setting LoudMax" << key << "to" << value;
            // }
        }
    }

    // Apply parameters to the plugin
    auto parameters = loudMaxPlugin->getParameters();
    for (int i = 0; i < parameters.size(); ++i) {
        auto* param = loudMaxPlugin->getHostedParameter(i);
        QString paramName = QString::fromStdString(param->getName(15).toStdString());
        
        if (paramValues.contains(paramName)) {
            float value = paramValues[paramName];
            param->setValue(value);
            param->sendValueChangedMessageToListeners(value);
        }
    }

    // qDebug() << "LoudMax parameters applied successfully.";
}

void MainWindow::updateFXButtonLED(bool active) {
    // qDebug() << "updateFXButtonLED called with" << active;
    if (currentFXButtonLEDState != active) {
        // qDebug() << "***** updateFXButtonLED changed" << active;
        currentFXButtonLEDState = active;
        QIcon ledIcon(active ? ":/graphics/led_red.png" : ":/graphics/led_grey.png");
        ui->FXbutton->setIcon(ledIcon);
        
        // Update font weight based on LED state
        QFont font = ui->FXbutton->font();
        font.setBold(active);
        ui->FXbutton->setFont(font);
    }
}

