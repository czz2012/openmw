#ifndef COMPONENTS_ESM_TERRAIN_STORAGE_H
#define COMPONENTS_ESM_TERRAIN_STORAGE_H

#include <cassert>
#include <mutex>

#include <components/terrain/storage.hpp>

#include <components/esm/esmterrain.hpp>
#include <components/esm/util.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/loadltex.hpp>

namespace ESM4
{
    struct Land;
}

namespace ESM
{
    class LandData;
}

namespace VFS
{
    class Manager;
}

namespace ESMTerrain
{

    class LandCache;

    /// @brief Wrapper around Land Data with reference counting. The wrapper needs to be held as long as the data is
    /// still in use
    class LandObject : public osg::Object
    {
    public:
        LandObject();
        LandObject(const ESM::Land* land, int loadFlags);
        LandObject(const ESM4::Land* land, int loadFlags);

        LandObject(const LandObject& copy, const osg::CopyOp& copyop);
        virtual ~LandObject();

        META_Object(ESMTerrain, LandObject)

        inline const ESM::LandData* getData(int flags) const
        {
            if ((mData.mLoadFlags & flags) != flags)
                return nullptr;

            return &mData;
        }
        inline int getPlugin() const { return mLand->getPlugin(); }
        inline int getLandSize() const { return mData.getLandSize(); }
        inline int getRealSize() const { return mData.getSize(); }

    private:
        const ESM::Land* mLand;

        ESM::LandData mData;
    };

    /// @brief Feeds data from ESM terrain records (ESM::Land, ESM::LandTexture)
    ///        into the terrain component, converting it on the fly as needed.
    class Storage : public Terrain::Storage
    {
    public:
        Storage(const VFS::Manager* vfs, const std::string& normalMapPattern = "",
            const std::string& normalHeightMapPattern = "", bool autoUseNormalMaps = false,
            const std::string& specularMapPattern = "", bool autoUseSpecularMaps = false);

        // Not implemented in this class, because we need different Store implementations for game and editor
        virtual osg::ref_ptr<const LandObject> getLand(ESM::ExteriorCellLocation cellLocation) = 0;
        virtual const ESM::LandTexture* getLandTexture(int index, short plugin) = 0;
        /// Get bounds of the whole terrain in cell units
        void getBounds(float& minX, float& maxX, float& minY, float& maxY, ESM::RefId worldspace) override = 0;

        /// Get the minimum and maximum heights of a terrain region.
        /// @note Will only be called for chunks with size = minBatchSize, i.e. leafs of the quad tree.
        ///        Larger chunks can simply merge AABB of children.
        /// @param size size of the chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param min min height will be stored here
        /// @param max max height will be stored here
        /// @return true if there was data available for this terrain chunk
        bool getMinMaxHeights(
            float size, const osg::Vec2f& center, ESM::RefId worldspace, float& min, float& max) override;

        /// Fill vertex buffers for a terrain chunk.
        /// @note May be called from background threads. Make sure to only call thread-safe functions from here!
        /// @note Vertices should be written in row-major order (a row is defined as parallel to the x-axis).
        ///       The specified positions should be in local space, i.e. relative to the center of the terrain chunk.
        /// @param lodLevel LOD level, 0 = most detailed
        /// @param size size of the terrain chunk in cell units
        /// @param center center of the chunk in cell units
        /// @param positions buffer to write vertices
        /// @param normals buffer to write vertex normals
        /// @param colours buffer to write vertex colours
        void fillVertexBuffers(int lodLevel, float size, const osg::Vec2f& center, ESM::RefId worldspace,
            osg::ref_ptr<osg::Vec3Array> positions, osg::ref_ptr<osg::Vec3Array> normals,
            osg::ref_ptr<osg::Vec4ubArray> colours) override;

        /// Create textures holding layer blend values for a terrain chunk.
        /// @note The terrain chunk shouldn't be larger than one cell since otherwise we might
        ///       have to do a ridiculous amount of different layers. For larger chunks, composite maps should be used.
        /// @note May be called from background threads.
        /// @param chunkSize size of the terrain chunk in cell units
        /// @param chunkCenter center of the chunk in cell units
        /// @param blendmaps created blendmaps will be written here
        /// @param layerList names of the layer textures used will be written here
        void getBlendmaps(float chunkSize, const osg::Vec2f& chunkCenter, ImageVector& blendmaps,
            std::vector<Terrain::LayerInfo>& layerList, ESM::RefId worldspace) override;

        float getHeightAt(const osg::Vec3f& worldPos, ESM::RefId worldspace) override;

        /// Get the transformation factor for mapping cell units to world units.
        float getCellWorldSize(ESM::RefId worldspace) override;

        /// Get the number of vertices on one side for each cell. Should be (power of two)+1
        int getCellVertices(ESM::RefId worldspace) override;

        int getBlendmapScale(float chunkSize) override;

        float getVertexHeight(const ESM::LandData* data, int x, int y)
        {
            const int landSize = data->getLandSize();
            assert(x < landSize);
            assert(y < landSize);
            return data->getHeights()[y * landSize + x];
        }

    private:
        const VFS::Manager* mVFS;

        inline void fixNormal(
            osg::Vec3f& normal, ESM::ExteriorCellLocation cellLocation, int col, int row, LandCache& cache);
        inline void fixColour(
            osg::Vec4ub& colour, ESM::ExteriorCellLocation cellLocation, int col, int row, LandCache& cache);
        inline void averageNormal(
            osg::Vec3f& normal, ESM::ExteriorCellLocation cellLocation, int col, int row, LandCache& cache);

        inline const LandObject* getLand(ESM::ExteriorCellLocation cellLocation, LandCache& cache);

        virtual bool useAlteration() const { return false; }
        virtual void adjustColor(int col, int row, const ESM::LandData* heightData, osg::Vec4ub& color) const;
        virtual float getAlteredHeight(int col, int row) const;

        // Since plugins can define new texture palettes, we need to know the plugin index too
        // in order to retrieve the correct texture name.
        // pair  <texture id, plugin id>
        typedef std::pair<short, short> UniqueTextureId;

        inline UniqueTextureId getVtexIndexAt(
            ESM::ExteriorCellLocation cellLocation, const LandObject* land, int x, int y, LandCache&);
        std::string getTextureName(UniqueTextureId id);

        std::map<std::string, Terrain::LayerInfo> mLayerInfoMap;
        std::mutex mLayerInfoMutex;

        std::string mNormalMapPattern;
        std::string mNormalHeightMapPattern;
        bool mAutoUseNormalMaps;

        std::string mSpecularMapPattern;
        bool mAutoUseSpecularMaps;

        Terrain::LayerInfo getLayerInfo(const std::string& texture);
    };

}

#endif
