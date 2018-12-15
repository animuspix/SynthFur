//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace SynthFur
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
		public:
			MainPage();
		private:
			void InitSettings(Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
			void InitTuringSettings(); // Turing setting initialization is just a helper method for reaction/diffusion
									   // initialization + reset, so it doesn't need any explicit arguments (at least
									   // not until/unless I add support for multiple reactions/reaction types)
			void MeshImporter(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void TexExportHelper(Windows::Graphics::DirectX::Direct3D11::IDirect3DSurface^ surf, Platform::String^ defaultFileName);
			void TuringTexExporter(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void MainTexExporter(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void TuringFadeRateUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringReactivityUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringDiffusionUUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringDiffusionVUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringConvRateUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringSupplyRateUpdate(Platform::Object ^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringKrnUpperLeftUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringKrnUpperMidUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringKrnUpperRightUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringKrnMidLeftUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringKrnMidMidUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringKrnMidRightUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringKrnLowerLeftUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringKrnLowerMidUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void TuringKrnLowerRightUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void FlipTuringShadingDirection(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void SwitchTuringPatterns(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void SwitchClampShading(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void BaseFadeRateUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void FlipBaseShadingDirection(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void AxisUpdateX(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void AxisUpdateY(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void AxisUpdateZ(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void AxisUpdateXTuring(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void AxisUpdateYTuring(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void EnableBothPalettes(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void EnablePaletteZero(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void EnablePaletteOne(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void AxisUpdateZTuring(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void BlendUpdateMul(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void BlendUpdateDiv(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void BlendUpdateAdd(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void BlendUpdateSub(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void ProjUpdateSurfUVs(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void ProjUpdateTriplanar(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void ProjUpdateFlatPlanar(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void TuringPatternReset(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void TuringParamReset(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void SwitchGPUWork(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
			void SwitchPatternsPaused(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void PauseSurfSpin(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e);
			void LiIntensityUpdate(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);
			void PaletteEdit(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void PaletteUpdate(Windows::UI::Xaml::Controls::ColorPicker^ sender, Windows::UI::Xaml::Controls::ColorChangedEventArgs^ args);
			void MeshImportHover(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
			void MeshImportLeave(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
			void ZoomView(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
			// Small event registration token, used to track init-settings event handler so we can remove it after
			// SynthFur initialization
			Windows::Foundation::EventRegistrationToken settingsInitToken;
	};
}
