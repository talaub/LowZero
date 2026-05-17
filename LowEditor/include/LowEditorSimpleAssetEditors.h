#pragma once

#include "LowEditorTypeEditor.h"
#include "LowEditorViewport.h"

namespace Low {
  namespace Editor {
    struct MeshAssetEditor : public TypeEditor
    {
      MeshAssetEditor(Util::Handle p_Handle);
      ~MeshAssetEditor();

      void render(const float p_Delta) override;

      virtual Math::UVector2 get_edit_widget_dimensions() override
      {
        return Math::UVector2{390, 600};
      }

    protected:
      MeshViewer *m_MeshViewer;
    };

    struct MaterialAssetEditor : public TypeEditor
    {
      MaterialAssetEditor(Util::Handle p_Handle);
      ~MaterialAssetEditor();

      void render(const float p_Delta) override;

      virtual Math::UVector2 get_edit_widget_dimensions() override
      {
        return Math::UVector2{390, 600};
      }

    protected:
      MaterialViewer *m_MaterialViewer;
    };

    struct TextureAssetEditor : public TypeEditor
    {
      TextureAssetEditor(Util::Handle p_Handle) : TypeEditor(p_Handle)
      {
      }

      void render(const float p_Delta) override;

      virtual Math::UVector2 get_edit_widget_dimensions() override
      {
        return Math::UVector2{390, 600};
      }
    };

    struct FontAssetEditor : public TypeEditor
    {
      FontAssetEditor(Util::Handle p_Handle) : TypeEditor(p_Handle)
      {
      }

      void render(const float p_Delta) override;

      virtual Math::UVector2 get_edit_widget_dimensions() override
      {
        return Math::UVector2{390, 180};
      }
    };
  } // namespace Editor
} // namespace Low
