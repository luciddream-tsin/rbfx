//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Core/Signal.h"
#include "../Graphics/Material.h"

namespace Urho3D
{

#if URHO3D_SYSTEMUI

struct MaterialTextureUnit
{
    bool desktop_{};
    TextureUnit unit_;
    ea::string name_;
    ea::string hint_;
};

/// SystemUI widget used to edit materials.
class URHO3D_API MaterialInspectorWidget : public Object
{
    URHO3D_OBJECT(MaterialInspectorWidget, Object);

public:
    Signal<void()> OnEditBegin;
    Signal<void()> OnEditEnd;

    using MaterialVector = ea::vector<SharedPtr<Material>>;

    MaterialInspectorWidget(Context* context, const MaterialVector& materials);
    ~MaterialInspectorWidget() override;

    void UpdateTechniques(const ea::string& path);

    void RenderTitle();
    void RenderContent();

    const MaterialVector& GetMaterials() const { return materials_; }

private:
    struct CachedTechnique
    {
        ea::string displayName_;
        ea::string resourceName_;
        SharedPtr<Technique> technique_;
        bool deprecated_{};

        bool operator<(const CachedTechnique& rhs) const;
    };
    using CachedTechniquePtr = ea::shared_ptr<CachedTechnique>;

    void UpdateCachedTechniques();
    void RenderTechniques();
    void RenderTextures();

    bool RenderTechniqueEntries();
    bool EditTechniqueInEntry(TechniqueEntry& entry, float itemWidth);
    bool EditDistanceInEntry(TechniqueEntry& entry, float itemWidth);
    bool EditQualityInEntry(TechniqueEntry& entry);

    void RenderTextureUnit(const MaterialTextureUnit& desc);
    Color GetLabelColor(const MaterialTextureUnit& desc, bool canEdit) const;

    ea::string GetTechniqueDisplayName(const ea::string& resourceName) const;
    bool IsTechniqueDeprecated(const ea::string& resourceName) const;

    const ea::string defaultTechniqueName_{"Techniques/LitOpaque.xml"};

    ea::unordered_map<ea::string, CachedTechniquePtr> techniques_;
    ea::vector<CachedTechniquePtr> sortedTechniques_;
    CachedTechniquePtr defaultTechnique_;

    MaterialVector materials_;
    ea::vector<TechniqueEntry> techniqueEntries_;
    ea::vector<TechniqueEntry> sortedTechniqueEntries_;

    bool pendingSetTechniques_{};
    ea::vector<ea::pair<TextureUnit, Texture*>> pendingSetTextures_;
};

#endif

}