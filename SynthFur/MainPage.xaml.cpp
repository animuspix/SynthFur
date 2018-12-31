//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <assert.h>
#include <string>
#include <cstdio>
#include <filesystem>

using namespace SynthFur;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Popups;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Graphics::Imaging;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Storage;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	// Initialize [this]
	InitializeComponent();

	// Acquire a C++/CX reference to the current window
	Window^ currWin = Window::Current;

	// Define [SynthFurTitle] as a custom title bar
	CoreApplicationViewTitleBar^ coreTitleBar = CoreApplication::GetCurrentView()->TitleBar;
	currWin->SetTitleBar(SynthFurTitle);
}

// Setting initializer for Turing-specific properties
void SynthFur::MainPage::InitTuringSettings()
{
	// Initialize Turing-specific properties to the values given in:
	// http://www.karlsims.com/rd.html
	TuringDiffRate0Slider2->Value = 500;
	TuringDiffRate1Slider3->Value = 250;
	TuringSupplyRateSlider5->Value = 27;
	TuringConvRateSlider4->Value = 31;
	TuringKrnLowerLeft->Value = 25;
	TuringKrnLowerRight->Value = 25;
	TuringKrnUpperLeft->Value = 25;
	TuringKrnUpperRight->Value = 25;
	TuringKrnUpperMid->Value = 100;
	TuringKrnLowerMid->Value = 100;
	TuringKrnMidLeft->Value = 100;
	TuringKrnMidRight->Value = 100;
	TuringKrnMidMid->Value = 0;
	MeshOut->InitTuringSettings();
}

// Default setting initializer for surface editor; queued after core SynthFur setup
// from inside [MeshImporter(...)] (see below)
void SynthFur::MainPage::InitSettings(Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	// Prepare default settings
	BothPalettes->IsChecked = true;
	ShadeAxisY->IsChecked = true;
	ShadeAxisYTuring->IsChecked = true;
	BlendOpMul->IsChecked = true;
	BaseGradSlider6->Value = 500;
	PatternGradSlider0->Value = 500;
	LiIntensitySlider7->Value = 500;
	ClampShading->IsChecked = true;
	ProjSurfaceUVs->IsChecked = true;
	InitTuringSettings();
	// Relevant event handlers may not invoke, so settings are applied directly through [MeshOut] as well
	// (occurs locally & automatically on the rendering thread, just mentioned here to simplify
	// maintenance)
	// We've initialized settings and propagated the changes
	// back to SynthFur, so remove the (programmatically set)
	// initializing event handler here
	MeshOut->SizeChanged -= settingsInitToken;
}

// Event handler for mesh imports
void SynthFur::MainPage::MeshImporter(Object^ sender, TappedRoutedEventArgs^ e)
{
	// Revert to standard arrow cursor
	Window::Current->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::Arrow, 0);

	// Open file picker, optionally read mesh + prepare texture simulation
	Pickers::FileOpenPicker^ picker = ref new Pickers::FileOpenPicker();
	picker->ViewMode = Pickers::PickerViewMode::List;
	picker->SuggestedStartLocation = Pickers::PickerLocationId::DocumentsLibrary;
	picker->FileTypeFilter->Append(".obj");
	Concurrency::create_task(picker->PickSingleFileAsync()).then([this](StorageFile^ file) // Asynchronously open file browser, select file, process (within lambda-function)
	{
		if (file != nullptr) // Only proceed for valid file selections
		{
			// The DirectXMesh wavefront reader seems to fail for full paths, and we don't have time to implement our own; copy
			// user meshes into a local file instead
			StorageFolder^ installDir = Windows::ApplicationModel::Package::Current->InstalledLocation;
			Concurrency::create_task([]() {}).then([installDir]() // Anonymous exception-capture task
			{
				return installDir->GetFileAsync(L"meshTmp.obj"); // Check if the temporary mesh file exists
			}).then([file, this, installDir](Concurrency::task<StorageFile^> t) // Task-based exception continuation
			{
				try
				{
					StorageFile^ tmp = t.get(); // No exceptions thrown, the file exists
					// Successful fetch, clean up leftover file and copy in selected mesh afterward
					Concurrency::create_task(tmp->DeleteAsync()).then([this, file, installDir]()
					{
						Concurrency::create_task(file->CopyAsync(installDir, L"meshTmp.obj")).then([this](StorageFile^ retTmp)
						{
							// Hide intro/landing page, display main page
							BodyLanding->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
							BodyMain->Visibility = Windows::UI::Xaml::Visibility::Visible;
							settingsInitToken = (MeshOut->SizeChanged += ref new Windows::UI::Xaml::SizeChangedEventHandler(this, &SynthFur::MainPage::InitSettings));
						});
					});
				}
				catch (Platform::COMException^ e)
				{
					// No existing temporary files, freely copy the selected mesh
					Concurrency::create_task(file->CopyAsync(installDir, L"meshTmp.obj")).then([this](StorageFile^ retTmp)
					{
						// Hide intro/landing page, display main page
						BodyLanding->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
						BodyMain->Visibility = Windows::UI::Xaml::Visibility::Visible;
					});
				}
			}).then([this, file]()
			{
				// Cache user mesh
				MeshOut->CacheMesh(L"meshTmp.obj");
				// Simulation setup occurs within the size-changed event attached to [MeshOut]
				// Update page title
				TitleLabel->Text = file->Name;
			});
		}
	});
}

// Small helper function to reduce work duplication between Turing pattern export and the main texture exporter
void SynthFur::MainPage::TexExportHelper(IDirect3DSurface^ surf, Platform::String^ defaultFileName)
{
	Concurrency::create_task(SoftwareBitmap::CreateCopyFromSurfaceAsync(surf, BitmapAlphaMode::Ignore)).then([this, surf, defaultFileName](SoftwareBitmap^ bmp)
	{
		// Initialize a file-save picker
		// Only bitmap and PNG export for now, more on request
		Pickers::FileSavePicker^ picker = ref new Pickers::FileSavePicker();
		picker->SuggestedStartLocation = Pickers::PickerLocationId::PicturesLibrary;
		// Cache arrays of extension aliases for each supported file-type
		Platform::Collections::Vector<Platform::String^>^ pngOptions = ref new Platform::Collections::Vector<Platform::String^>();
		pngOptions->Append(".png");
		pngOptions->Append(".PNG");
		Platform::Collections::Vector<Platform::String^>^ bmpOptions = ref new Platform::Collections::Vector<Platform::String^>();
		pngOptions->Append(".bmp");
		pngOptions->Append(".BMP");
		// Insert collective .png and .bmp aliases as specific file-type choices
		picker->FileTypeChoices->Insert(".png", pngOptions);
		picker->FileTypeChoices->Insert("Uncompressed bitmap", bmpOptions);
		picker->SuggestedFileName = defaultFileName;

		// Concurrently save with the chosen name/extension at the chosen folder
		Concurrency::create_task(picker->PickSaveFileAsync()).then([this, surf, defaultFileName, bmp](StorageFile^ file) // Asynchronously open file browser, select file, process (within lambda-function)
		{
			if (file != nullptr) // Only commit to saving if the user hasn't cancelled
			{
				// Encode to .png or .bmp (lambdas are sandboxed, so these must be segregated)
				Concurrency::create_task(file->OpenAsync(FileAccessMode::ReadWrite)).then([this, surf, defaultFileName, file, bmp](Streams::IRandomAccessStream^ stream)
				{
					// Choose betwen .bmp and .png encoders
					Platform::Guid encoderID;
					if (file->FileType == L".png" ||
						file->FileType == L".PNG")
					{
						encoderID = BitmapEncoder::BmpEncoderId;
					}
					else if (file->FileType == L".bmp" ||
							 file->FileType == L".BMP")
					{
						encoderID = BitmapEncoder::PngEncoderId;
					}

					// Set generic encoder properties
					Concurrency::create_task(BitmapEncoder::CreateAsync(BitmapEncoder::BmpEncoderId, stream)).then([this, surf, defaultFileName, file, bmp](BitmapEncoder^ encoder)
					{
						encoder->SetSoftwareBitmap(bmp); // Prepare bitmap for output
						// Concurrently write to the save location with thumbnailing active
						auto writeTask = Concurrency::create_task(encoder->FlushAsync());
						writeTask.then([this, surf, defaultFileName, file, writeTask]()
						{
							try
							{
								writeTask.get(); // Task results; exceptions on these tell us the enclosing [task] failed somewhere
								ContentDialog^ success = ref new ContentDialog(); // Only reached if [writeTask.get()] doesn't throw
								success->Title = "Success";
								success->Content = "File saved successfully";
								success->CloseButtonText = "OK";
							}
							catch (Platform::COMException^ e)
							{
								// Initialize saving error message
								ContentDialog^ unsaved = ref new ContentDialog();
								unsaved->Title = "Failed to save " + file->DisplayName + " with file-type " + file->FileType + ".";
								unsaved->Content = "Encoding error message: " + e->Message + "\n You may be able to try again with another file-type or a different name.";
								unsaved->PrimaryButtonText = "Try again?";
								unsaved->CloseButtonText = "Cancel";
								unsaved->DefaultButton = ContentDialogButton::Primary;

								// Respond to cancel/retry options appropriately
								Concurrency::create_task(unsaved->ShowAsync()).then([this, surf, defaultFileName, file](ContentDialogResult result)
								{
									if (result == ContentDialogResult::Primary)
									{
										// Re-start file save
										TexExportHelper(surf, defaultFileName);
									}
									// The user chose not to immediately try again; leave the saving function
								});
							}
						});
					});
				});
			}
		});
	});
}

// Event handler for Turing texture export
void SynthFur::MainPage::TuringTexExporter(Object^ sender, TappedRoutedEventArgs^ e)
{
	IDirect3DSurface^ surf = MeshOut->ExportTex((int)DXPanel::EXPORTABLE_TEX_IDS::TURING);
	TexExportHelper(surf, "grayScottImg");
}

// Event handler for main texture export
void SynthFur::MainPage::MainTexExporter(Object^ sender, TappedRoutedEventArgs^ e)
{
	IDirect3DSurface^ surf = MeshOut->ExportTex((int)DXPanel::EXPORTABLE_TEX_IDS::OUTPUT);
	TexExportHelper(surf, "synthFurTexture");
}

void SynthFur::MainPage::SwapPopout(Object^ sender, TappedRoutedEventArgs ^ e)
{
	bool collapsed = (bool)ProjOptionsPopout->Visibility;
	ProjOptionsPopout->Visibility = (Windows::UI::Xaml::Visibility)(!collapsed); // Show/hide the pop-out options menu
	// Constrain grid sizes to prevent overlap between the title-label and the options menu
	OptionsPopoutCell->MinWidth = collapsed ? 220.0 : 0.0;
}

void SynthFur::MainPage::TuringFadeRateUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract fractional slider value
	float valf = (float)(e->NewValue / ((((Slider^)sender)->Maximum + 1.0) / 2.0f));
	MeshOut->SetTuringFadeRate(valf);
}

void SynthFur::MainPage::TuringReactivityUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract fractional slider value
	float valf = (float)(e->NewValue / ((((Slider^)sender)->Maximum + 1.0) / 2.0f));
	MeshOut->SetTuringReactivity(valf);
}

void SynthFur::MainPage::TuringDiffusionUUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract fractional slider value
	float valf = (float)(e->NewValue / ((((Slider^)sender)->Maximum + 1.0) / 2.0f));
	MeshOut->SetTuringDiffusionU(valf);
}

void SynthFur::MainPage::TuringDiffusionVUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract fractional slider value
	float valf = (float)(e->NewValue / ((((Slider^)sender)->Maximum + 1.0) / 2.0f));
	MeshOut->SetTuringDiffusionV(valf);
}

void SynthFur::MainPage::TuringConvRateUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract fractional slider value
	float valf = (float)(e->NewValue / ((((Slider^)sender)->Maximum + 1.0) / 2.0f));
	MeshOut->SetTuringConvRate(valf);
}

void SynthFur::MainPage::TuringSupplyRateUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract fractional slider value
	float valf = (float)(e->NewValue / ((((Slider^)sender)->Maximum + 1.0) / 2.0f));
	MeshOut->SetTuringFeedRate(valf);
}

void SynthFur::MainPage::TuringKrnMidMidUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract fractional slider value, scale to [-1000...1000]
	MeshOut->SetTuringKrnCenter((((int)(e->NewValue)) * 2) - DXPanel::RATIONAL_DENOM);
}

void SynthFur::MainPage::TuringKrnMidLeftUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract slider value, scale to [0...1000]
	MeshOut->SetTuringKrnElt(((int)(e->NewValue)) * 2, 0);
}

void SynthFur::MainPage::TuringKrnMidRightUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract slider value, scale to [0...1000]
	MeshOut->SetTuringKrnElt(((int)(e->NewValue)) * 2, 1);
}

void SynthFur::MainPage::TuringKrnUpperMidUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract slider value, scale to [0...1000]
	MeshOut->SetTuringKrnElt(((int)(e->NewValue)) * 2, 2);
}

void SynthFur::MainPage::TuringKrnLowerMidUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract slider value, scale to [0...1000]
	MeshOut->SetTuringKrnElt(((int)(e->NewValue)) * 2, 3);
}

void SynthFur::MainPage::TuringKrnUpperRightUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract slider value, scale to [0...1000]
	MeshOut->SetTuringKrn2Elt(((int)(e->NewValue)) * 2, 0);
}

void SynthFur::MainPage::TuringKrnLowerLeftUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract slider value, scale to [0...1000]
	MeshOut->SetTuringKrn2Elt(((int)(e->NewValue)) * 2, 1);
}

void SynthFur::MainPage::TuringKrnLowerRightUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract slider value, scale to [0...1000]
	MeshOut->SetTuringKrn2Elt(((int)(e->NewValue)) * 2, 2);
}

void SynthFur::MainPage::TuringKrnUpperLeftUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract slider value, scale to [0...1000]
	MeshOut->SetTuringKrn2Elt(((int)(e->NewValue)) * 2, 3);
}

void SynthFur::MainPage::FlipTuringShadingDirection(Object^ sender, RoutedEventArgs^ e)
{
	MeshOut->SwitchTuringShadingDirection();
}

void SynthFur::MainPage::SwitchTuringPatterns(Object^ sender, RoutedEventArgs^ e)
{
	MeshOut->SwitchTuringPatterns();
}

void SynthFur::MainPage::SwitchClampShading(Object^ sender, RoutedEventArgs^ e)
{
	MeshOut->SwitchSurfaceClamping();
}

void SynthFur::MainPage::BaseFadeRateUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract fractional slider value
	float valf = (float)(e->NewValue / ((((Slider^)sender)->Maximum + 1.0) / 2.0f));
	MeshOut->SetBaseFadeRate(valf);
}

void SynthFur::MainPage::FlipBaseShadingDirection(Object^ sender, RoutedEventArgs^ e)
{
	MeshOut->SwitchBaseShadingDirection();
}

void SynthFur::MainPage::AxisUpdateX(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetBaseShadingAxis(0);
}

void SynthFur::MainPage::AxisUpdateY(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetBaseShadingAxis(1);
}

void SynthFur::MainPage::AxisUpdateZ(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetBaseShadingAxis(2);
}

void SynthFur::MainPage::AxisUpdateXTuring(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringShadingAxis(0);
}

void SynthFur::MainPage::AxisUpdateYTuring(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringShadingAxis(1);
}

void SynthFur::MainPage::AxisUpdateZTuring(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringShadingAxis(2);
}

void SynthFur::MainPage::EnableBothPalettes(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetBaseFadeState(0);
}

void SynthFur::MainPage::EnablePaletteZero(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetBaseFadeState(1);
}

void SynthFur::MainPage::EnablePaletteOne(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetBaseFadeState(2);
}

void SynthFur::MainPage::BlendUpdateMul(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringBlendOp(0);
}

void SynthFur::MainPage::BlendUpdateDiv(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringBlendOp(1);
}

void SynthFur::MainPage::BlendUpdateAdd(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringBlendOp(2);
}

void SynthFur::MainPage::BlendUpdateSub(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringBlendOp(3);
}

void SynthFur::MainPage::ProjUpdateSurfUVs(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringProjection(0);
}

void SynthFur::MainPage::ProjUpdateTriplanar(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringProjection(1);
}

void SynthFur::MainPage::ProjUpdateFlatPlanar(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SetTuringProjection(2);
}

void SynthFur::MainPage::TuringPatternReset(Object^ sender, RoutedEventArgs^ e)
{
	// Generate confirmation window here
	ContentDialog^ resetCheck = ref new ContentDialog();
	resetCheck->Title = "Are you sure?";
	resetCheck->Content = "Resetting pattern synthesis will discard existing patterns and restart the generation process.";
	resetCheck->PrimaryButtonText = "Yes, reset pattern synthesis";
	resetCheck->CloseButtonText = "No, maintain existing patterns";
	resetCheck->DefaultButton = ContentDialogButton::Close;

	// Respond to cancel/retry options appropriately
	Concurrency::create_task(resetCheck->ShowAsync()).then([this](ContentDialogResult result)
	{
		if (result == ContentDialogResult::Primary)
		{
			// Reset pattern synthesis
			MeshOut->TuringReset();
		}
		PatternReset->IsChecked = false; // Uncheck the reset button
		// The user chose not to reset, so exit the pattern-reset function here
	});
}

void SynthFur::MainPage::TuringParamReset(Object^ sender, RoutedEventArgs^ e)
{
	// Generate confirmation window here
	ContentDialog^ resetCheck = ref new ContentDialog();
	resetCheck->Title = "Are you sure?";
	resetCheck->Content = "Resetting reaction/diffusion settings will revert any parameter changes you've applied to the defaults used on startup.";
	resetCheck->PrimaryButtonText = "Yes, reset pattern settings";
	resetCheck->CloseButtonText = "No, maintain existing settings";
	resetCheck->DefaultButton = ContentDialogButton::Close;

	// Respond to cancel/retry options appropriately
	Concurrency::create_task(resetCheck->ShowAsync()).then([this](ContentDialogResult result)
	{
		if (result == ContentDialogResult::Primary)
		{
			// Reset pattern settings
			InitTuringSettings();
			MeshOut->InitTuringSettings();
		}
		ParamReset->IsChecked = false; // Uncheck the reset button
		// The user chose not to reset, so exit the settings-reset function here
	});
}

void SynthFur::MainPage::SwitchGPUWork(Object^ sender, RoutedEventArgs^ e)
{
	MeshOut->SwitchTexSim();
}

void SynthFur::MainPage::SwitchPatternsPaused(Object^ sender, RoutedEventArgs^ e)
{
	MeshOut->SwitchPatternSim();
}

void SynthFur::MainPage::PauseSurfSpin(Object^ sender, TappedRoutedEventArgs^ e)
{
	MeshOut->SwitchSurfSpin();
}

void SynthFur::MainPage::LiIntensityUpdate(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
	// Extract fractional slider value
	float valf = (float)(e->NewValue / ((((Slider^)sender)->Maximum + 1.0) / 2.0f));
	MeshOut->SetLiIntensity(valf);
}

void SynthFur::MainPage::PaletteEdit(Object^ sender, TappedRoutedEventArgs^ e)
{
	// Cache the name of palette-box tapped by the user
	Platform::String^ name = ((Shapes::Rectangle^)(e->OriginalSource))->Name;
	// Strip out, cast ID value to integer
	wchar_t strID[2] = { name->Data()[name->Length() - 1], ' ' };
	int id = std::stoi(strID);
	// Select corresponding picker & display if it's not already visible;
	// hide if it was visible when the event was emitted (=> implies the user's
	// finished selecting a color and wants to leave)
	// Also hide neighbouring pickers
	switch (id)
	{
		case 0:
			PalettePicker0->Visibility = Windows::UI::Xaml::Visibility(!((bool)((PalettePicker0->Visibility))));
			PalettePicker1->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
			PalettePicker2->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
			break;
		case 1:
			PalettePicker1->Visibility = Windows::UI::Xaml::Visibility(!((bool)((PalettePicker1->Visibility))));
			PalettePicker0->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
			PalettePicker2->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
			break;
		case 2:
			PalettePicker2->Visibility = Windows::UI::Xaml::Visibility(!((bool)((PalettePicker2->Visibility))));
			PalettePicker0->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
			PalettePicker1->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
			break;
		default:
			assert(false); // Picker-ID out of range
			break;
	}
	// Adjust spacing around palette boxes + title
	if (!(bool(PalettePicker0->Visibility)) ||
		!(bool(PalettePicker1->Visibility)) ||
		!(bool(PalettePicker2->Visibility)))
	{
		// Allow extra spacing for color pickers
		PaletteTitle->Padding = Thickness(14, 14, 0, 14);
		PalettePanel->Padding = Thickness(14, 28, 14, 0);
	}
	else
	{
		// Reset spacing if all pickers are closed
		PaletteTitle->Padding = Thickness(14, 14, 0, 0);
		PalettePanel->Padding = Thickness(14, 8, 14, 0);
	}
}

void SynthFur::MainPage::PaletteUpdate(ColorPicker^ sender, ColorChangedEventArgs^ args)
{
	// Extract picker-ID
	Platform::String^ name = sender->Name;
	wchar_t strID[2] = { name->Data()[name->Length() - 1], ' ' };
	int id = std::stoi(strID);
	// Update palettes appropriately
	switch (id)
	{
		case 0:
			// Transpose colors from ARGB to RGBA (clipping of A since we aren't doing alpha-blending)
			MeshOut->SetFurPalette0(args->NewColor.R,
									args->NewColor.G,
									args->NewColor.B);
			break;
		case 1:
			// Transpose colors from ARGB to RGBA (clipping of A since we aren't doing alpha-blending)
			MeshOut->SetFurPalette1(args->NewColor.R,
									args->NewColor.G,
									args->NewColor.B);
			break;
		case 2:
			// Transpose colors from ARGB to RGBA (clipping of A since we aren't doing alpha-blending)
			MeshOut->SetTuringPalette(args->NewColor.R,
									  args->NewColor.G,
									  args->NewColor.B);
			break;
		default:
			assert(false); // Setting ID out of range
	}
}

void SynthFur::MainPage::FauxButtonHover(Object^ sender, PointerRoutedEventArgs^ e)
{
	// Select click-hint cursor
	Window::Current->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::Hand, 0);
}

void SynthFur::MainPage::FauxButtonLeave(Object^ sender, PointerRoutedEventArgs^ e)
{
	// Revert to standard arrow cursor
	Window::Current->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::Arrow, 0);
}

void SynthFur::MainPage::ZoomView(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	Windows::UI::Input::PointerPoint^ pp = e->GetCurrentPoint((SwapChainPanel^)sender); // Cache pointer information
	MeshOut->UpdateViewZoom(pp->Properties->MouseWheelDelta); // Pass scroll-wheel delta along to SynthFur
}

