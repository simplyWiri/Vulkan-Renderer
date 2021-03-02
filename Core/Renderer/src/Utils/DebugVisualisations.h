#pragma once
#include <map>
#include <vector>

#include "implot/implot.h"
#include "imgui/imgui.h"

namespace Renderer
{
#if DEBUG

	namespace ImPlotVar
	{
		struct PlotVarData
		{
			ImGuiID ID;
			std::vector<float> Data;
			std::vector<float> x;
			float lastData;
			int DataInsertIdx;
			int LastFrame;

			PlotVarData() : ID(0), lastData(0), DataInsertIdx(0), LastFrame(-1) {}
		};

		typedef std::map<ImGuiID, PlotVarData> PlotVarsMap;
		static PlotVarsMap g_PlotVarsMap;

		void PlotVar(const char* label, float value, float scale_min = FLT_MAX, float scale_max = FLT_MAX, size_t buffer_size = 120, bool addVal = true, const char* suffix = "")
		{
			ImGui::PushID(label);
			ImGuiID id = ImGui::GetID(label);

			PlotVarData& pvd = g_PlotVarsMap[id];

			if (pvd.Data.capacity() != buffer_size)
			{
				pvd.Data.resize(buffer_size);
				pvd.x.resize(buffer_size);
				memset(&pvd.Data[0], 0, sizeof(float) * buffer_size);
				pvd.DataInsertIdx = 0;
				pvd.LastFrame = -1;
			}

			if (pvd.DataInsertIdx == buffer_size) pvd.DataInsertIdx = 0;

			if (addVal)
			{
				pvd.Data[pvd.DataInsertIdx++] = value;
				pvd.lastData = value;
			}

			int current_frame = ImGui::GetFrameCount();

			ImPlot::SetNextPlotLimits(0,buffer_size ,0,40);
			
			if (pvd.LastFrame != current_frame)
			{
				ImGui::PlotLines("##plot", &pvd.Data[0], buffer_size, pvd.DataInsertIdx, NULL, scale_min, scale_max, ImVec2(0, 40));
				ImGui::SameLine();
				ImGui::Text("%s\n%-3.4f%s", label, value, suffix);	// Display last value in buffer
				pvd.LastFrame = current_frame;
			}

			ImGui::PopID();
		}
	}

	void DrawDebugVisualisations(/*Core* core, FrameInfo& frameInfo, const std::vector<std::unique_ptr<RenderGraph::PassDesc>>& passes*/)
	{
		//ImGui::NewFrame();

		//ImGui::SetNextWindowPos({ 5, 5 });
		//ImGui::Begin("Information");

		//ImGui::Text("Width: %d, Height: %d, VSync: %s", core->GetSettings()->width, core->GetSettings()->height, core->GetSettings()->vsync ? "True" : "False");
		//ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / static_cast<float>(frameInfo.fps)), frameInfo.fps);

		//float framesDiff = frameInfo.frameIndex - core->GetSwapchain()->prevFrames;
		//if (framesDiff >= 5)
		//{
		//	long long timeDiff = frameInfo.time - core->GetSwapchain()->prevFpsSample;
		//	auto frameTime = timeDiff / (framesDiff + FLT_EPSILON);
		//	ImPlotVar::PlotVar("Frame Times", frameTime, FLT_MAX, FLT_MAX, 144, frameInfo.frameIndex % 7 == 0, "ms");
		//}
		//else { ImPlotVar::PlotVar("Frame Times", FLT_MAX, FLT_MAX, FLT_MAX, 144, false, "ms"); }


		//if (ImGui::CollapsingHeader("RenderGraph"))
		//{
		//	int i = 0;
		//	ImGui::BeginChild("RenderPasses");

		//	ImGui::Columns(2, "Render Passes", true);
		//	ImGui::Text("Name");
		//	ImGui::NextColumn();
		//	ImGui::Text("Order");
		//	ImGui::NextColumn();
		//	ImGui::Separator();

		//	static int selected = -1;

		//	for (auto& pass : passes)
		//	{
		//		ImGui::BeginGroup();
		//		char label[32];
		//		sprintf_s(label, "%s", pass->GetName().c_str());
		//		if (ImGui::Selectable(label, selected == i, ImGuiSelectableFlags_SpanAllColumns)) selected = selected == i ? -1 : i;

		//		if (ImGui::IsItemHovered()) ImGui::SetTooltip("View Statistics");

		//		ImGui::NextColumn();
		//		ImGui::Text("%d", i++);
		//		ImGui::NextColumn();
		//		ImGui::Separator();
		//		ImGui::EndGroup();
		//	}

		//	ImGui::EndChild();
		//}

		//if (ImGui::CollapsingHeader("Allocations")) { core->GetAllocator()->DebugView(); }

		//ImGui::End();
	}

#else
	void DrawDebugVisualisations(Core* core, FrameInfo& frameInfo, const std::vector<RenderGraph::PassDesc> passes)
	{
		ImGui::NewFrame();

		ImGui::SetNextWindowPos({ 5, 5 });
		ImGui::Begin("Information");

		ImGui::Text("Width: %d, Height: %d, VSync: %s", core->GetSettings()->width, core->GetSettings()->height, core->GetSettings()->vsync ? "True" : "False");
		ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / static_cast<float>(frameInfo.fps)), frameInfo.fps);

		ImGui::End();
	}
#endif
}
