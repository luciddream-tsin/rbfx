//
// Copyright (c) 2022-2022 the rbfx project.
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

#include <Urho3D/Precompiled.h>

#include <Urho3D/IO/BinaryArchive.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/SceneResource.h>
#include <Urho3D/Resource/BinaryFile.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLArchive.h>
#include <Urho3D/Resource/XMLFile.h>

namespace Urho3D
{

SceneResource::SceneResource(Context* context)
    : Resource(context)
    , scene_(MakeShared<Scene>(context))
{
}

SceneResource::~SceneResource()
{
}

void SceneResource::RegisterObject(Context* context)
{
    context->AddFactoryReflection<SceneResource>();
}

void SceneResource::SetSaveFormatHint(InternalResourceFormat format)
{
    saveFormat_ = format != InternalResourceFormat::Unknown ? ea::make_optional(format) : ea::nullopt;
}

bool SceneResource::Save(Serializer& dest, InternalResourceFormat format, bool asPrefab) const
{
    if (asPrefab)
    {
        // Save as prefab is rare, we can afford to be suboptimal here.
        PrefabResource prefab(context_);
        scene_->GeneratePrefab(prefab.GetMutableScenePrefab());
        prefab.NormalizeIds();
        return prefab.Save(dest, format);
    }

    try
    {
        switch (format)
        {
        case InternalResourceFormat::Json:
        {
            JSONFile jsonFile(context_);
            JSONOutputArchive archive{context_, jsonFile.GetRoot(), &jsonFile};
            {
                ArchiveBlock block = archive.OpenUnorderedBlock(GetXmlRootName());
                scene_->SerializeInBlock(archive, false, PrefabSaveFlag::EnumsAsStrings, PrefabLoadFlag::None);
            }
            return jsonFile.Save(dest);
        }
        case InternalResourceFormat::Xml:
        {
            XMLFile xmlFile(context_);
            XMLOutputArchive archive{context_, xmlFile.GetOrCreateRoot(GetXmlRootName()), &xmlFile};
            {
                ArchiveBlock block = archive.OpenUnorderedBlock(GetXmlRootName());
                scene_->SerializeInBlock(archive, false, PrefabSaveFlag::EnumsAsStrings, PrefabLoadFlag::None);
            }
            return xmlFile.Save(dest);
        }
        case InternalResourceFormat::Binary:
        {
            dest.Write(DefaultBinaryMagic.data(), BinaryMagicSize);

            BinaryOutputArchive archive{context_, dest};
            {
                ArchiveBlock block = archive.OpenUnorderedBlock(GetXmlRootName());
                scene_->SerializeInBlock(archive, false, PrefabSaveFlag::CompactAttributeNames, PrefabLoadFlag::None);
            }
            return true;
        }
        default:
        {
            URHO3D_ASSERT(false);
            return false;
        }
        }
    }
    catch (const ArchiveException& e)
    {
        URHO3D_LOGERROR("Cannot save SimpleResource: ", e.what());
        return false;
    }
}

bool SceneResource::SaveFile(const FileIdentifier& fileName, InternalResourceFormat format, bool asPrefab) const
{
    auto vfs = GetSubsystem<VirtualFileSystem>();
    auto file = vfs->OpenFile(fileName, FILE_WRITE);
    return file && Save(*file, format, asPrefab);
}

bool SceneResource::BeginLoad(Deserializer& source)
{
    loadBinaryFile_ = nullptr;
    loadJsonFile_ = nullptr;
    loadXmlFile_ = nullptr;

    const auto format = PeekResourceFormat(source, DefaultBinaryMagic);
    switch (format)
    {
    case InternalResourceFormat::Json:
    {
        loadJsonFile_ = MakeShared<JSONFile>(context_);
        if (!loadJsonFile_->Load(source))
            return false;

        loadFormat_ = format;
        return true;
    }
    case InternalResourceFormat::Xml:
    {
        loadXmlFile_ = MakeShared<XMLFile>(context_);
        if (!loadXmlFile_->Load(source))
            return false;

        loadFormat_ = format;
        return true;
    }
    case InternalResourceFormat::Binary:
    {
        loadBinaryFile_ = MakeShared<BinaryFile>(context_);
        loadBinaryFile_->Load(source);

        loadFormat_ = format;
        return true;
    }
    default:
    {
        URHO3D_LOGERROR("Unknown resource format");
        loadFormat_ = InternalResourceFormat::Unknown;
        return false;
    }
    }
}

bool SceneResource::EndLoad()
{
    if (!loadFormat_ || loadFormat_ == InternalResourceFormat::Unknown)
        return false;

    try
    {
        bool cancelReload = false;
        OnReloadBegin(this, cancelReload);

        if (!cancelReload)
        {
            switch (*loadFormat_)
            {
            case InternalResourceFormat::Json:
            {
                JSONInputArchive archive{context_, loadJsonFile_->GetRoot(), loadJsonFile_};
                ArchiveBlock block = archive.OpenUnorderedBlock(GetXmlRootName());
                scene_->SerializeInBlock(archive, false, PrefabSaveFlag::None, PrefabLoadFlag::None);
                break;
            }
            case InternalResourceFormat::Xml:
            {
                XMLElement xmlRoot = loadXmlFile_->GetRoot();
                if (xmlRoot.GetName() == GetXmlRootName())
                {
                    XMLInputArchive archive{context_, xmlRoot, loadXmlFile_};
                    ArchiveBlock block = archive.OpenUnorderedBlock(GetXmlRootName());
                    scene_->SerializeInBlock(archive, false, PrefabSaveFlag::None, PrefabLoadFlag::None);
                }
                else
                {
                    if (!scene_->LoadXML(xmlRoot))
                        throw ArchiveException("Cannot load Scene from legacy XML format");
                }
                break;
            }
            case InternalResourceFormat::Binary:
            {
                MemoryBuffer readBuffer{loadBinaryFile_->GetData()};
                readBuffer.SeekRelative(BinaryMagicSize);

                BinaryInputArchive archive{GetContext(), readBuffer};
                ArchiveBlock block = archive.OpenUnorderedBlock(GetXmlRootName());
                scene_->SerializeInBlock(archive, false, PrefabSaveFlag::None, PrefabLoadFlag::None);

                break;
            }
            default:
            {
                throw ArchiveException("Cannot load Scene from the file of unknown format");
            }
            }
        }

        loadJsonFile_ = nullptr;
        loadBinaryFile_ = nullptr;
        loadXmlFile_ = nullptr;

        OnReloadEnd(this, !cancelReload);

        return true;
    }
    catch (const ArchiveException& e)
    {
        URHO3D_LOGERROR("Cannot load SimpleResource: ", e.what());
        return false;
    }
}

bool SceneResource::Save(Serializer& dest) const
{
    const auto format = saveFormat_.value_or(loadFormat_.value_or(InternalResourceFormat::Xml));
    return Save(dest, format, isPrefab_);
}

bool SceneResource::SaveFile(const FileIdentifier& fileName) const
{
    const auto format = saveFormat_.value_or(loadFormat_.value_or(InternalResourceFormat::Xml));
    return SaveFile(fileName, format, isPrefab_);
}

const char* SceneResource::GetXmlRootName()
{
    return "resource";
}

} // namespace Urho3D
