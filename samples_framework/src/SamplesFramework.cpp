/***********************************************************************
created:    24/5/2012
author:     Lukas E Meindl
*************************************************************************/
/***************************************************************************
*   Copyright (C) 2004 - 2013 Paul D Turner & The CEGUI Development Team
*
*   Permission is hereby granted, free of charge, to any person obtaining
*   a copy of this software and associated documentation files (the
*   "Software"), to deal in the Software without restriction, including
*   without limitation the rights to use, copy, modify, merge, publish,
*   distribute, sublicense, and/or sell copies of the Software, and to
*   permit persons to whom the Software is furnished to do so, subject to
*   the following conditions:
*
*   The above copyright notice and this permission notice shall be
*   included in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
*   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
*   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
*   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
*   OTHER DEALINGS IN THE SOFTWARE.
***************************************************************************/
#include "SamplesFramework.h"

#include "Sample.h"
#include "SampleData.h"
#include "CEGuiBaseApplication.h"

#include "MetaDataWindowManager.h"
#include "SamplesBrowserManager.h"

#include "CEGUI/CEGUI.h"
#include "CEGUI/Logger.h"
#include "CEGUI/widgets/PushButton.h"
#include "CEGUI/widgets/ProgressBar.h"

#include "samples.h"

#include <string>
#include <iostream>
#include <vector>

using namespace CEGUI;

//platform-dependant DLL delay-loading includes
#if (defined( __WIN32__ ) || defined( _WIN32 ))
#include "windows.h"
#endif

//----------------------------------------------------------------------------//
int main(int argc, char* argv[])
{
    // Basic start-up for the sample browser application.
    // Will remain in run() until quitting
    int argidx = 1;
#ifdef __APPLE__
    if (argc > 1 && !std::strncmp(argv[argidx], "-psn", 4))
    {
        --argc;
        ++argidx;
    }
#endif

    SamplesFramework sampleFramework;
    return sampleFramework.run();
}

//----------------------------------------------------------------------------//
SamplesFramework::SamplesFramework() :
    d_sampleExitButton(0),
    d_metaDataWinMgr(0),
    d_samplesWinMgr(0),
    d_selectedSampleData(0),
    d_loadingProgressBar(0),
    d_quittingSampleView(false)
{
}

//----------------------------------------------------------------------------//
SamplesFramework::~SamplesFramework()
{
    if (d_metaDataWinMgr)
        delete d_metaDataWinMgr;
}

//----------------------------------------------------------------------------//
bool SamplesFramework::initialise()
{
    using namespace CEGUI;

    initialiseLoadScreenLayout();

    loadSamples();

    d_systemInputAggregator = new InputAggregator(
        &CEGUI::System::getSingletonPtr()->getDefaultGUIContext());
    d_systemInputAggregator->initialise();

    // return true to signalize the initialisation was sucessful and run the
    // SamplesFramework
    return true;
}

//----------------------------------------------------------------------------//
void SamplesFramework::deinitialise()
{
    unloadSamples();

    if (d_systemInputAggregator != 0)
    {
        delete d_systemInputAggregator;
        d_systemInputAggregator = 0;
    }
}

//----------------------------------------------------------------------------//
void SamplesFramework::initialiseLoadScreenLayout()
{
    CEGUI::Font& font =
        FontManager::getSingleton().createFromFile("DejaVuSans-12.font");

    CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultFont(&font);

    SchemeManager::getSingleton().createFromFile("SampleBrowser.scheme");

    WindowManager& winMgr(WindowManager::getSingleton());
    Window* loadScreenRoot =
        winMgr.loadLayoutFromFile("SampleBrowserLoadScreen.layout");
    System::getSingleton().getDefaultGUIContext().setRootWindow(loadScreenRoot);

    d_loadingProgressBar = static_cast<CEGUI::ProgressBar*>(
        loadScreenRoot->getChild("LoadScreenProgressBar"));
    d_loadingScreenText = loadScreenRoot->getChild("LoadScreenText");
    d_loadScreenChunkProgressText =
        d_loadingScreenText->getChild("LoadScreenTextChunkProgress");

    d_loadingProgressBar->setProgress(0.0f);
}

//----------------------------------------------------------------------------//
void SamplesFramework::loadSamples()
{
#   include "samples.inc"
    
    std::vector<Sample*>::iterator iter = samples.begin();
    
    for (; iter != samples.end(); ++iter)
    {
        addSampleDataCppModule(*iter, (*iter)->getName(), (*iter)->getSummary(),
                               (*iter)->getDescription(), (*iter)->getType(), (*iter)->getCredits());
    }
}

//----------------------------------------------------------------------------//
void SamplesFramework::unloadSamples()
{
    while (d_samples.size() > 0)
    {
        SampleData*& sampleData = d_samples.back();

        sampleData->deinitialise();
        delete sampleData;

        d_samples.pop_back();
    }
}

//----------------------------------------------------------------------------//
void SamplesFramework::addSampleDataCppModule(Sample* instance,
											  CEGUI::String sampleName,
                                              CEGUI::String summary,
                                              CEGUI::String description,
                                              SampleType sampleTypeEnum,
                                              CEGUI::String credits)
{
    SampleData* sampleData = new SampleDataModule(instance, sampleName, summary,
                                                  description, sampleTypeEnum,
                                                  credits);

    addSample(sampleData);
}

//----------------------------------------------------------------------------//
bool SamplesFramework::injectKeyDown(const CEGUI::Key::Scan& ceguiKey)
{
    if (Key::Escape != ceguiKey)
        return getCurrentInputAggregator()->injectKeyDown(ceguiKey);
    else
    {
        if (d_selectedSampleData)
            stopDisplaySample();
        else
            setQuitting(true);
    }

    return false;
}

//----------------------------------------------------------------------------//
bool SamplesFramework::injectKeyUp(const CEGUI::Key::Scan& ceguiKey)
{
    if (getCurrentInputAggregator() == 0)
        return false;

    return getCurrentInputAggregator()->injectKeyUp(ceguiKey);
}

//----------------------------------------------------------------------------//
bool SamplesFramework::injectChar(int character)
{
    if (getCurrentInputAggregator() == 0)
        return false;

    return getCurrentInputAggregator()->injectChar(character);
}

//----------------------------------------------------------------------------//
bool SamplesFramework::injectMouseButtonDown(
                                    const CEGUI::MouseButton& ceguiMouseButton)
{
    if (getCurrentInputAggregator() == 0)
        return false;

    return getCurrentInputAggregator()->injectMouseButtonDown(ceguiMouseButton);
}

//----------------------------------------------------------------------------//
bool SamplesFramework::injectMouseButtonUp(
                                    const CEGUI::MouseButton& ceguiMouseButton)
{
    if (getCurrentInputAggregator() == 0)
        return false;

    return getCurrentInputAggregator()->injectMouseButtonUp(ceguiMouseButton);
}

//----------------------------------------------------------------------------//
bool SamplesFramework::injectMouseWheelChange(float position)
{
    if (getCurrentInputAggregator() == 0)
        return false;

    return getCurrentInputAggregator()->injectMouseWheelChange(position);
}

//----------------------------------------------------------------------------//
bool SamplesFramework::injectMousePosition(float x, float y)
{
    if (getCurrentInputAggregator() == 0)
        return false;

    return getCurrentInputAggregator()->injectMousePosition(x, y);
}

//----------------------------------------------------------------------------//
void SamplesFramework::update(float passedTime)
{
    static bool init(false);

    if (!init)
    {
        init = updateInitialisationStep();

        CEGUI::GUIContext& defaultGUIContext(
            CEGUI::System::getSingleton().getDefaultGUIContext());
        defaultGUIContext.injectTimePulse(passedTime);
    }
    else
    {
        if (d_quittingSampleView)
            stopDisplaySample();

        if (!d_selectedSampleData)
        {
            updateSamples(passedTime);

            CEGUI::GUIContext& defaultGUIContext(
                CEGUI::System::getSingleton().getDefaultGUIContext());
            defaultGUIContext.injectTimePulse(passedTime);
        }
        else
        {
            d_selectedSampleData->getGuiContext()->injectTimePulse(passedTime);

            d_selectedSampleData->update(passedTime);
        }
    }

}

//----------------------------------------------------------------------------//
void SamplesFramework::handleNewWindowSize(float width, float height)
{
    d_appWindowWidth = static_cast<int>(width);
    d_appWindowHeight = static_cast<int>(height);

    std::vector<SampleData*>::iterator iter = d_samples.begin();
    std::vector<SampleData*>::iterator end = d_samples.end();
    for (; iter != end; ++iter)
    {
        SampleData* sampleData = *iter;

        sampleData->handleNewWindowSize(width, height);
    }

    if (d_samplesWinMgr)
        d_samplesWinMgr->setWindowRatio(width / height);
}

//----------------------------------------------------------------------------//
void SamplesFramework::addSample(SampleData* sampleData)
{
    d_samples.push_back(sampleData);
}

//----------------------------------------------------------------------------//
void SamplesFramework::renderGUIContexts()
{
    if (!d_selectedSampleData)
    {
        renderSampleGUIContexts();

        CEGUI::System& gui_system(CEGUI::System::getSingleton());
        gui_system.getDefaultGUIContext().draw();
    }
    else
    {
        d_selectedSampleData->getGuiContext()->draw();
    }

}

//----------------------------------------------------------------------------//
void SamplesFramework::handleSampleSelection(CEGUI::Window* sampleWindow)
{
    SampleData* correspondingSampleData = findSampleData(sampleWindow);

    d_metaDataWinMgr->setSampleInfo(correspondingSampleData);
}

//----------------------------------------------------------------------------//
void SamplesFramework::handleStartDisplaySample(CEGUI::Window* sampleWindow)
{
    SampleData* correspondingSampleData = findSampleData(sampleWindow);

    CEGUI::RenderTarget& defaultRenderTarget =
        CEGUI::System::getSingleton().getRenderer()->getDefaultRenderTarget();
    CEGUI::GUIContext* sampleContext(correspondingSampleData->getGuiContext());
    sampleContext->setRenderTarget(defaultRenderTarget);

    sampleContext->getRootWindow()->addChild(d_sampleExitButton);
    sampleContext->getPointerIndicator().setPosition(
        CEGUI::System::getSingleton().getDefaultGUIContext().getPointerIndicator().
            getPosition());

    d_selectedSampleData = correspondingSampleData;

    d_selectedSampleData->onEnteringSample();
}

//----------------------------------------------------------------------------//
void SamplesFramework::stopDisplaySample()
{
    GUIContext* sampleGUIContext = d_selectedSampleData->getGuiContext();

    // Since we switch our contexts, the mouse release won't be injected if we
    // don't do it manually
    if (getCurrentInputAggregator() != 0)
        getCurrentInputAggregator()->injectMouseButtonUp(CEGUI::LeftButton);
    sampleGUIContext->injectTimePulse(0.0f);

    sampleGUIContext->getRootWindow()->removeChild(d_sampleExitButton);
    d_selectedSampleData->setGUIContextRTT();

    CEGUI::System::getSingleton().getDefaultGUIContext().getPointerIndicator().
        setPosition(sampleGUIContext->getPointerIndicator().getPosition());

    d_selectedSampleData = 0;
    d_quittingSampleView = false;
}

//----------------------------------------------------------------------------//
SampleData* SamplesFramework::findSampleData(CEGUI::Window* sampleWindow)
{
    //Find corresponding SampleData
    std::vector<SampleData*>::iterator iter = d_samples.begin();
    std::vector<SampleData*>::iterator end = d_samples.end();
    for (; iter != end; ++iter)
    {
        SampleData* sampleData = *iter;

        if (sampleData->getSampleWindow() == sampleWindow)
            return sampleData;
    }

    return 0;
}

//----------------------------------------------------------------------------//
bool SamplesFramework::handleSampleExitButtonClicked(const CEGUI::EventArgs& args)
{
    d_quittingSampleView = true;

    return true;
}

//----------------------------------------------------------------------------//
bool SamplesFramework::initialiseSampleStepwise(int sampleNumber)
{
    if (static_cast<int>(d_samples.size()) <= sampleNumber)
        return true;

    if (static_cast<int>(d_samples.size()) > sampleNumber + 1)
        displaySampleLoadProgress(sampleNumber);

    if (sampleNumber >= 0)
    {
        SampleData* sampleData = d_samples[sampleNumber];
        sampleData->initialise(d_appWindowWidth, d_appWindowHeight);
        CEGUI::FrameWindow* sampleWindow = d_samplesWinMgr->createSampleWindow(
            sampleData->getName(), sampleData->getRTTImage());

        sampleData->setSampleWindow(sampleWindow);

        d_baseApp->registerSampleOverlayHandler(sampleData->getGuiContext());
    }

    return false;
}

//----------------------------------------------------------------------------//
void SamplesFramework::initialiseSampleBrowserLayout()
{
    CEGUI::FontManager::getSingleton().
        createFreeTypeFont("DejaVuSans-14", 14.f, true, "DejaVuSans.ttf");

    WindowManager& winMgr(WindowManager::getSingleton());

    CEGUI::FontManager::getSingleton().
        createFromFile("DejaVuSans-10-NoScale.font");
    CEGUI::FontManager::getSingleton().
        createFromFile("Junicode-13.font");

    CEGUI::SchemeManager::getSingleton().createFromFile("Generic.scheme");

    if (!ImageManager::getSingleton().isDefined("BackgroundSampleBrowser"))
        ImageManager::getSingleton().addBitmapImageFromFile(
            "BackgroundSampleBrowser", "BackgroundSampleBrowser.jpg");

    d_root = winMgr.loadLayoutFromFile("SampleBrowser.layout");
    d_root->getChild("BackgroundImage")->setProperty(
        "Image", "BackgroundSampleBrowser");

    CEGUI::Window* metaDataWindow = d_root->getChild("SampleBrowserMetaData");
    d_metaDataWinMgr = new MetaDataWindowManager(metaDataWindow);

    CEGUI::Window* samplesScrollablePane = d_root->getChild(
        "SampleFrameWindowContainer/SamplesFrameWindow/SamplesScrollablePane");
    d_samplesWinMgr = new SamplesBrowserManager(this, samplesScrollablePane);

    d_sampleExitButton = static_cast<CEGUI::PushButton*>(winMgr.createWindow(
        "SampleBrowserSkin/Button", "SampleExitButton"));

    d_sampleExitButton->setSize(CEGUI::USize(cegui_absdim(34.f),
                                             cegui_absdim(34.f)));
    d_sampleExitButton->setPosition(CEGUI::UVector2(cegui_absdim(0.0f),
                                                    cegui_absdim(0.0f)));
    d_sampleExitButton->setHorizontalAlignment(HA_RIGHT);
    d_sampleExitButton->setVerticalAlignment(VA_TOP);
    d_sampleExitButton->setProperty(
        "NormalImage", "SampleBrowserSkin/ExitButtonNormal");
    d_sampleExitButton->setProperty(
        "HoverImage", "SampleBrowserSkin/ExitButtonHover");
    d_sampleExitButton->setProperty(
        "PushedImage", "SampleBrowserSkin/ExitButtonClicked");
    d_sampleExitButton->subscribeEvent(
        PushButton::EventClicked,
        Event::Subscriber(&SamplesFramework::handleSampleExitButtonClicked, this));

    d_sampleExitButton->setAlwaysOnTop(true);
}

//----------------------------------------------------------------------------//
bool SamplesFramework::updateInitialisationStep()
{
    static int step(0);

    switch (step)
    {
    case 0:
    {
        ++step;
        displaySampleBrowserLayoutLoadProgress();

        break;
    }

    case 1:
    {
        initialiseSampleBrowserLayout();
        ++step;
        break;
    }

    default:
    {
        bool sampleInitFinished = initialiseSampleStepwise(step - 3); // -2 for the previous 2 steps, -1 for extra step to display the text before actually loading
        if (sampleInitFinished)
        {
            //Loading finished, switching layout to sample browser
            initialisationFinalisation();
            return true;
        }
        else
            ++step;

        break;
    }
    }

    return false;
}

//----------------------------------------------------------------------------//
void SamplesFramework::initialisationFinalisation()
{
    System::getSingleton().getDefaultGUIContext().getPointerIndicator().
        setDefaultImage("SampleBrowserSkin/MouseArrow");
    d_samplesWinMgr->setWindowRatio(d_appWindowWidth / (float)d_appWindowHeight);

    System::getSingleton().getDefaultGUIContext().setRootWindow(d_root);

    if (d_samples.size() > 0)
    {
        if (CEGUI::Window* wnd = d_samples[0]->getSampleWindow())
        {
            handleSampleSelection(wnd);
            d_samplesWinMgr->selectSampleWindow(wnd);
        }
    }
}

//----------------------------------------------------------------------------//
void SamplesFramework::updateSamples(float passedTime)
{
    std::vector<SampleData*>::iterator iter = d_samples.begin();
    std::vector<SampleData*>::iterator end = d_samples.end();
    for (; iter != end; ++iter)
    {
        SampleData* sampleData = *iter;

        GUIContext* guiContext = sampleData->getGuiContext();
        guiContext->injectTimePulse(passedTime);

        sampleData->update(passedTime);
    }
}

//----------------------------------------------------------------------------//
void SamplesFramework::renderSampleGUIContexts()
{
    std::vector<SampleData*>::iterator iter = d_samples.begin();
    std::vector<SampleData*>::iterator end = d_samples.end();
    for (; iter != end; ++iter)
    {
        SampleData* sampleData = *iter;

        if (!sampleData->getGuiContext())
            continue;

        CEGUI::Window* sampleWindow = sampleData->getSampleWindow();

        if (!areWindowsIntersecting(d_samplesWinMgr->getWindow(), sampleWindow))
            continue;

        if (sampleData->getGuiContext()->isDirty())
        {
            sampleData->getSampleWindow()->invalidate();
            sampleData->clearRTTTexture();
            sampleData->getGuiContext()->draw();
        }
    }
}

//----------------------------------------------------------------------------//
void SamplesFramework::displaySampleBrowserLayoutLoadProgress()
{
    int totalNum = d_samples.size() + 2;

    CEGUI::String loadText = CEGUI::String("Loading SampleBrowser skin ...");
    d_loadingScreenText->setText(loadText);

    CEGUI::String progressText =  PropertyHelper<int>::toString(1) + "/" +
        PropertyHelper<int>::toString(totalNum - 1);
    d_loadScreenChunkProgressText->setText(progressText);

    d_loadingProgressBar->setProgress(1.0f / (totalNum - 1.0f));
}

//----------------------------------------------------------------------------//
void SamplesFramework::displaySampleLoadProgress(int sampleNumber)
{
    SampleData* sampleData = d_samples[sampleNumber + 1];

    int totalNum = d_samples.size() + 2;
    CEGUI::String loadText = "Loading " + sampleData->getName() + " ...";
    d_loadingScreenText->setText(loadText);

    CEGUI::String progressText = PropertyHelper<int>::toString(sampleNumber + 3) +
        "/" + PropertyHelper<int>::toString(totalNum - 1);
    d_loadScreenChunkProgressText->setText(progressText);

    d_loadingProgressBar->setProgress( (sampleNumber + 3.f) / (totalNum - 1.0f) );
}

//----------------------------------------------------------------------------//
bool SamplesFramework::areWindowsIntersecting(CEGUI::Window* window1,
                                              CEGUI::Window* window2)
{
    const CEGUI::Rectf& clipRect1 = window1->getOuterRectClipper();
    const CEGUI::Rectf& clipRect2 = window2->getOuterRectClipper();

    return  clipRect1.left() < clipRect2.right()
        && clipRect1.right() > clipRect2.left()
        && clipRect1.top() < clipRect2.bottom()
        && clipRect1.bottom() > clipRect2.top();
}

//----------------------------------------------------------------------------//
CEGUI::InputAggregator* SamplesFramework::getCurrentInputAggregator()
{
    if (d_selectedSampleData != 0)
        return d_selectedSampleData->getInputAggregator();

    return d_systemInputAggregator;
}

//----------------------------------------------------------------------------//