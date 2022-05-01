/**********************************************************************************************
*
*   rres-raylib v1.0 - rres loaders specific for raylib data structures
*
*   CONFIGURATION:
*
*   #define RRES_RAYLIB_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers
*       or source files without problems. But only ONE file should hold the implementation.
*
*   #define RRES_SUPPORT_COMPRESSION_LZ4
*       Support data compression LZ4 algorithm, provided by lz4.h/lz4.c library
* 
*   #define RRES_SUPPORT_ENCRYPTION_MONOCYPHER
*       Support data encryption algorithms provided by monocypher.h/monocypher.c library
* 
* 
*   LICENSE: MIT
*
*   Copyright (c) 2020-2022 Ramon Santamaria (@raysan5)
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
**********************************************************************************************/

#ifndef RRES_RAYLIB_H
#define RRES_RAYLIB_H

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------

// rres data loading to raylib data structures
RLAPI void *LoadDataFromResource(rresResource rres, int *size);     // Load raw data from rres resource
RLAPI char *LoadTextFromResource(rresResource rres);                // Load text data from rres resource
RLAPI Image LoadImageFromResource(rresResource rres);               // Load Image data from rres resource
RLAPI Wave LoadWaveFromResource(rresResource rres);                 // Load Wave data from rres resource
RLAPI Font LoadFontFromResource(rresResource rres);                 // Load Font data from rres resource
RLAPI Mesh LoadMeshFromResource(rresResource rres);                 // Load Mesh data from rres resource
//RLAPI Material LoadMaterialFromResource(rresResource rres);       // TODO: Load Material data from rres resource
//RLAPI Model LoadModelFromResource(rresResource rres);             // TODO: Load Model data from rres resource

#endif // RRES_RAYLIB_H

/***********************************************************************************
*
*   RRES RAYLIB IMPLEMENTATION
*
************************************************************************************/

#if defined(RRES_RAYLIB_IMPLEMENTATION)

// Include supported compression/encryption algorithms
// NOTE: They should be the same supported by the rres packaging tool (rrespacker)
#if defined(RRES_SUPPORT_COMPRESSION_LZ4)
    #include "external/lz4.h"               // LZ4 compression algorithm
    #include "external/lz4.c"               // LZ4 compression algorithm implementation
#endif
#if defined(RRES_SUPPORT_ENCRYPTION_MONOCYPHER)
    #include "external/monocypher.h"        // Encryption algorithms
    #include "external/monocypher.c"        // Encryption algorithms implementation
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------

// Load simple data chunks that are later required by multi-chunk resources
static void *LoadDataFromResourceChunk(rresResourceChunk chunk, int *size);     // Load chunk: RRES_DATA_RAW
static char *LoadTextFromResourceChunk(rresResourceChunk chunk);                // Load chunk: RRES_DATA_TEXT
static Image LoadImageFromResourceChunk(rresResourceChunk chunk);               // Load chunk: RRES_DATA_IMAGE

static int UnpackDataFromResourceChunk(rresResourceChunk *chunk);   // Unpack compressed/encrypted data from chunk
                                                                    // NOTE: Function return 0 on success or other value on failure

static const char *GetFourCCFromType(unsigned int type)             // Get FourCC 4-char code from resource type, useful for log info

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Load raw data from rres resource
void *LoadDataFromResource(rresResource rres, int *size)
{
    void *data = NULL;

    if ((rres.count >= 1) && (rres.chunks[0].type == RRES_DATA_RAW))
    {
        data = LoadDataFromResourceChunk(rres.chunks[0], size);
    }

    return data;
}

// Load text data from rres resource
// NOTE: Text must be NULL terminated
char *LoadTextFromResource(rresResource rres)
{
    char *text = NULL;

    if ((rres.count >= 1) && (rres.chunks[0].type == RRES_DATA_TEXT))
    {
        text = LoadTextFromResourceChunk(rres.chunks[0]);
    }

    return text;
}

// Load Image data from rres resource
Image LoadImageFromResource(rresResource rres)
{
    Image image = { 0 };

    if ((rres.count >= 1) && (rres.chunks[0].type == RRES_DATA_IMAGE))
    {
        image = LoadImageFromResourceChunk(rres.chunks[0]);
    }

    return image;
}

// Load Wave data from rres resource
Wave LoadWaveFromResource(rresResource rres)
{
    Wave wave = { 0 };

    if ((rres.count >= 1) && (rres.chunks[0].type == RRES_DATA_WAVE))
    {
        int result = UnpackDataFromResourceChunk(&rres.chunks[0]);

        if (result == 0)    // Data was successfully decompressed/decrypted
        {
            wave.frameCount = rres.chunks[0].props[0];
            wave.sampleRate = rres.chunks[0].props[1];
            wave.sampleSize = rres.chunks[0].props[2];
            wave.channels = rres.chunks[0].props[3];

            unsigned int size = wave.frameCount*wave.sampleSize/8;
            wave.data = RL_CALLOC(size);
            memcpy(wave.data, rres.chunks[0].data, size);
        }
    }

    return wave;
}

// Load Font data from rres resource
Font LoadFontFromResource(rresResource rres)
{
    Font font = { 0 };

    // Font resource consist of (2) chunks:
    //  - RRES_DATA_GLYPH_INFO: Basic font and glyphs properties/data
    //  - RRES_DATA_IMAGE: Image atlas for the font characters
    if (rres.count >= 2)
    {
        if (rres.chunks[0].type == RRES_DATA_GLYPH_INFO)
        {
            int result = UnpackDataFromResourceChunk(&rres.chunks[0]);

            if (result == 0)    // Data was successfully decompressed/decrypted
            {
                // Load font basic properties from chunk[0]
                font.baseSize = rres.chunks[0].props[0];           // Base size (default chars height)
                font.glyphCount = rres.chunks[0].props[1];         // Number of characters (glyphs)
                font.glyphPadding = rres.chunks[0].props[2];      // Padding around the chars

                font.recs = (Rectangle *)RL_CALLOC(font.glyphCount*sizeof(Rectangle));
                font.glyphs = (GlyphInfo *)RL_CALLOC(font.glyphCount*sizeof(GlyphInfo));

                for (int i = 0; i < font.glyphCount; i++)
                {
                    // Font glyphs info comes as a data blob
                    font.recs[i].x = (float)((rresFontGlyphInfo *)rres.chunks[0].data)[i].x;
                    font.recs[i].y = (float)((rresFontGlyphInfo *)rres.chunks[0].data)[i].y;
                    font.recs[i].width = (float)((rresFontGlyphInfo *)rres.chunks[0].data)[i].width;
                    font.recs[i].height = (float)((rresFontGlyphInfo *)rres.chunks[0].data)[i].height;

                    font.glyphs[i].value = ((rresFontGlyphInfo *)rres.chunks[0].data)[i].value;
                    font.glyphs[i].offsetX = ((rresFontGlyphInfo *)rres.chunks[0].data)[i].offsetX;
                    font.glyphs[i].offsetY = ((rresFontGlyphInfo *)rres.chunks[0].data)[i].offsetY;
                    font.glyphs[i].advanceX = ((rresFontGlyphInfo *)rres.chunks[0].data)[i].advanceX;

                    // NOTE: font.glyphs[i].image is not loaded
                }
            }
        }

        // Load font image chunk
        if (rres.chunks[1].type == RRES_DATA_IMAGE)
        {
            Image image = LoadImageFromResourceChunk(rres.chunks[1]);
            font.texture = LoadTextureFromImage(image);
            UnloadImage(image);
        }
    }

    return font;
}

// Load Mesh data from rres resource
// NOTE: We try to load vertex data following raylib structure constraints,
// in case data does not fit raylib Mesh structure, it is not loaded
Mesh LoadMeshFromResource(rresResource rres)
{
    Mesh mesh = { 0 };
    
    // Mesh resource consist of (n) chunks:
    for (int i = 0; i < rres.count; i++)
    {
        result = UnpackDataFromResourceChunk(&rres.chunks[i]);
        
        if (result == 0)
        {
            // NOTE: raylib only supports vertex arrays with same vertex count,
            // rres.chunks[0] defined vertexCount will be the reference for the following chunks
            // The only exception to vertexCount is the mesh.indices array
            if (mesh.vertexCount == 0) mesh.vertexCount = rres.chunks[0].props[0];
            
            // Verify chunk type and vertex count
            if (rres.chunks[i].type == RRES_DATA_VERTEX)
            {
                // In case vertex count do not match we skip that resource chunk
                if ((rres.chunks[i].props[1] != RRES_VERTEX_ATTRIBUTE_INDEX) && (rres.chunks[i].props[0] != mesh.vertexCount)) continue;
                
                // NOTE: We are only loading raylib supported rresVertexFormat and raylib expected components count
                switch (rres.chunks[i].props[1])    // Check rresVertexAttribute value
                {
                    case RRES_VERTEX_ATTRIBUTE_POSITION: 
                    {
                        // raylib expects 3 components per vertex and float vertex format
                        if ((rres.chunks[i].props[2] == 3) && (rres.chunks[i].props[3] == RRES_VERTEX_FORMAT_FLOAT)) 
                        {
                            mesh.vertices = (float *)RL_CALLOC(mesh.vertexCount*3*sizeof(float));
                            memcpy(mesh.vertices, rres.chunks[i].data, mesh.vertexCount*3*sizeof(float));
                        }
                        else RRES_LOG("WARNING: MESH: Vertex attribute position not valid, componentCount/vertexFormat do not fit\n");
                        
                    } break;
                    case RRES_VERTEX_ATTRIBUTE_TEXCOORD1: 
                    {
                        // raylib expects 2 components per vertex and float vertex format
                        if ((rres.chunks[i].props[2] == 2) && (rres.chunks[i].props[3] == RRES_VERTEX_FORMAT_FLOAT) 
                        {
                            mesh.texcoords = (float *)RL_CALLOC(mesh.vertexCount*2*sizeof(float));
                            memcpy(mesh.texcoords, rres.chunks[i].data, mesh.vertexCount*2*sizeof(float));
                        }
                        else RRES_LOG("WARNING: MESH: Vertex attribute texcoord1 not valid, componentCount/vertexFormat do not fit\n");
                        
                    } break;
                    case RRES_VERTEX_ATTRIBUTE_TEXCOORD2: 
                    {
                        // raylib expects 2 components per vertex and float vertex format
                        if ((rres.chunks[i].props[2] == 2) && (rres.chunks[i].props[3] == RRES_VERTEX_FORMAT_FLOAT) 
                        {
                            mesh.texcoords2 = (float *)RL_CALLOC(mesh.vertexCount*2*sizeof(float));
                            memcpy(mesh.texcoords2, rres.chunks[i].data, mesh.vertexCount*2*sizeof(float));
                        }
                        else RRES_LOG("WARNING: MESH: Vertex attribute texcoord2 not valid, componentCount/vertexFormat do not fit\n");
                        
                    } break;
                    case RRES_VERTEX_ATTRIBUTE_TEXCOORD3: 
                    {
                        RRES_LOG("WARNING: MESH: Vertex attribute texcoord3 not supported\n");
                        
                    } break;
                    case RRES_VERTEX_ATTRIBUTE_TEXCOORD4:
                    {
                        RRES_LOG("WARNING: MESH: Vertex attribute texcoord4 not supported\n");
                        
                    } break;
                    case RRES_VERTEX_ATTRIBUTE_NORMAL: 
                    {
                        // raylib expects 3 components per vertex and float vertex format
                        if ((rres.chunks[i].props[2] == 3) && (rres.chunks[i].props[3] == RRES_VERTEX_FORMAT_FLOAT)) 
                        {
                            mesh.normals = (float *)RL_CALLOC(mesh.vertexCount*3*sizeof(float));
                            memcpy(mesh.normals, rres.chunks[i].data, mesh.vertexCount*3*sizeof(float));
                        }
                        else RRES_LOG("WARNING: MESH: Vertex attribute normal not valid, componentCount/vertexFormat do not fit\n");
                        
                    } break;
                    case RRES_VERTEX_ATTRIBUTE_TANGENT: 
                    {
                        // raylib expects 4 components per vertex and float vertex format
                        if ((rres.chunks[i].props[2] == 4) && (rres.chunks[i].props[3] == RRES_VERTEX_FORMAT_FLOAT)) 
                        {
                            mesh.tangents = (float *)RL_CALLOC(mesh.vertexCount*4*sizeof(float));
                            memcpy(mesh.tangents, rres.chunks[i].data, mesh.vertexCount*4*sizeof(float));
                        }
                        else RRES_LOG("WARNING: MESH: Vertex attribute tangent not valid, componentCount/vertexFormat do not fit\n");
                        
                    } break;
                    case RRES_VERTEX_ATTRIBUTE_COLOR: 
                    {
                        // raylib expects 4 components per vertex and unsigned char vertex format
                        if ((rres.chunks[i].props[2] == 4) && (rres.chunks[i].props[3] == RRES_VERTEX_FORMAT_UBYTE)) 
                        {
                            mesh.colors = (unsigned char *)RL_CALLOC(mesh.vertexCount*4*sizeof(unsigned char));
                            memcpy(mesh.colors, rres.chunks[i].data, mesh.vertexCount*4*sizeof(unsigned char));
                        }
                        else RRES_LOG("WARNING: MESH: Vertex attribute color not valid, componentCount/vertexFormat do not fit\n");
                        
                    } break;
                    case RRES_VERTEX_ATTRIBUTE_INDEX: 
                    {
                        // raylib expects 1 components per index and unsigned short vertex format
                        if ((rres.chunks[i].props[2] == 1) && (rres.chunks[i].props[3] == RRES_VERTEX_FORMAT_USHORT)) 
                        {
                            mesh.indices = (unsigned char *)RL_CALLOC(rres.chunks[i].props[0]*sizeof(unsigned short));
                            memcpy(mesh.indices, rres.chunks[i].data, rres.chunks[i].props[0]*sizeof(unsigned short));
                        }
                        else RRES_LOG("WARNING: MESH: Vertex attribute index not valid, componentCount/vertexFormat do not fit\n");
                        
                    } break;
                    default: break;
                }
            }
        }
    }

    return mesh;
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------

// Load data chunk: RRES_DATA_RAW
// NOTE: This chunk can be used raw files embedding or other binary blobs
static void *LoadDataFromResourceChunk(rresResourceChunk chunk, int *size);
{
    void *rawData = NULL;

    if (chunk.type == RRES_DATA_RAW)
    {
        int result = UnpackDataFromResourceChunk(&rres.chunks[0]);

        if (result == 0)    // Data was successfully decompressed/decrypted
        {
            rawData = RL_CALLOC(chunk.props[0], 1);
            memcpy(rawData, chunk.data, chunk.props[0]);
            *size = chunk.props[0];
        }
    }

    return rawData;
}

// Load data chunk: RRES_DATA_TEXT
// NOTE: This chunk can be used for shaders or other text data elements (materials?)
static char *LoadTextFromResourceChunk(rresResourceChunk chunk)
{
    void *text = NULL;

    if (chunk.type == RRES_DATA_TEXT)
    {
        int result = UnpackDataFromResourceChunk(&rres.chunks[0]);

        if (result == 0)    // Data was successfully decompressed/decrypted
        {
            text = (char *)RL_CALLOC(chunk.props[0] + 1, 1);    // We add NULL terminator, just in case
            memcpy(text, chunk.data, chunk.props[0]);
            
            // TODO: We got some extra text properties, in case they could be useful for users:
            // chunk.props[1]:rresTextEncoding, chunk.props[2]:rresCodeLang, chunk. props[3]:cultureCode
        }
    }

    return text;
}

// Load data chunk: RRES_DATA_IMAGE
// NOTE: Many data types use images data in some way (font, material...)
static Image LoadImageFromResourceChunk(rresResourceChunk chunk)
{
    Image image = { 0 };

    if (chunk.type == RRES_DATA_IMAGE)
    {
        int result = UnpackDataFromResourceChunk(&rres.chunks[0]);

        if (result == 0)    // Data was successfully decompressed/decrypted
        {
            image.width = chunk.props[0];
            image.height = chunk.props[1];
            int format = chunk.props[2];
            
            // Assign equivalent pixel formats for our engine
            // NOTE: In this case rresPixelFormat defined values match raylib PixelFormat values
            switch (format)
            {
                case RRES_PIXELFORMAT_UNCOMP_GRAYSCALE: image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE; break;
                case RRES_PIXELFORMAT_UNCOMP_GRAY_ALPHA: image.format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA; break;
                case RRES_PIXELFORMAT_UNCOMP_R5G6B5: image.format = PIXELFORMAT_UNCOMPRESSED_R5G5B5A1; break;
                case RRES_PIXELFORMAT_UNCOMP_R8G8B8: image.format = PIXELFORMAT_UNCOMPRESSED_R5G6B5; break;
                case RRES_PIXELFORMAT_UNCOMP_R5G5B5A1: image.format = PIXELFORMAT_UNCOMPRESSED_R4G4B4A4; break;
                case RRES_PIXELFORMAT_UNCOMP_R4G4B4A4: image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8; break;
                case RRES_PIXELFORMAT_UNCOMP_R8G8B8A8: image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8; break;
                case RRES_PIXELFORMAT_UNCOMP_R32: image.format = PIXELFORMAT_UNCOMPRESSED_R32; break;
                case RRES_PIXELFORMAT_UNCOMP_R32G32B32: image.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32; break;
                case RRES_PIXELFORMAT_UNCOMP_R32G32B32A32: image.format = PIXELFORMAT_UNCOMPRESSED_R32G32B32A32; break;
                case RRES_PIXELFORMAT_COMP_DXT1_RGB: image.format = PIXELFORMAT_COMPRESSED_DXT1_RGB; break;  
                case RRES_PIXELFORMAT_COMP_DXT1_RGBA: image.format = PIXELFORMAT_COMPRESSED_DXT1_RGBA; break;
                case RRES_PIXELFORMAT_COMP_DXT3_RGBA: image.format = PIXELFORMAT_COMPRESSED_DXT3_RGBA; break;
                case RRES_PIXELFORMAT_COMP_DXT5_RGBA: image.format = PIXELFORMAT_COMPRESSED_DXT5_RGBA; break;
                case RRES_PIXELFORMAT_COMP_ETC1_RGB: image.format = PIXELFORMAT_COMPRESSED_ETC1_RGB; break;
                case RRES_PIXELFORMAT_COMP_ETC2_RGB: image.format = PIXELFORMAT_COMPRESSED_ETC2_RGB; break;
                case RRES_PIXELFORMAT_COMP_ETC2_EAC_RGBA: image.format = PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA; break;
                case RRES_PIXELFORMAT_COMP_PVRT_RGB: image.format = PIXELFORMAT_COMPRESSED_PVRT_RGB; break;
                case RRES_PIXELFORMAT_COMP_PVRT_RGBA: image.format = PIXELFORMAT_COMPRESSED_PVRT_RGBA; break;
                case RRES_PIXELFORMAT_COMP_ASTC_4x4_RGBA: image.format = PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA; break;
                case RRES_PIXELFORMAT_COMP_ASTC_8x8_RGBA: image.format = PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA; break;
                default: break;
            }
            
            image.mipmaps = chunk.props[3];

            // Image data size can be computed from image properties
            unsigned int size = GetPixelDataSize(image.width, image.height, image.format);
            
            // NOTE: Computed image data must match the data size of the chunk processed (minus props size)
            if (size == (chunk.baseSize - 20))
            {
                image.data = RL_CALLOC(size);
                memcpy(image.data, chunk.data, size);
            }
            else RRES_LOG("WARNING: IMGE: Chunk data size do not match expected image data size\n");
        }
    }

    return image;
}

// Unpack compressed/encrypted data from resource chunk
// NOTE 1: Function return 0 on success or an error code on failure
// NOTE 2: Data corruption CRC32 check has already been performed by rresLoadResource() on rres.h
static int UnpackDataFromResourceChunk(rresResourceChunk *chunk)
{
    int result = 0;

    // Result error codes:
    //  0 - No error, decompression/decryption successful
    //  1 - Encryption algorithm not supported
    //  2 - Invalid password on decryption
    //  3 - Compression algorithm not supported
    //  4 - Error on data decompression

    // NOTE 1: If data is compressed/encrypted the properties are not loaded by rres.h because
    // it's up to the user to process the data; *chunk must be properly updated by this function
    // NOTE 2: rres-raylib should support the same algorithms and libraries used by rrespacker tool

    // STEP 1. Decrypt message if encrypted
    if (chunk->cipherType != RRES_CIPHER_NONE)
    {
        if (chunk->cipherType == RRES_CIPHER_XCHACHA20_POLY1305)    // rrespacker supported encryption
        {
#if defined(RRES_SUPPORT_ENCRYPTION_MONOCYPHER)
            // Get some memory for the possible message output
            unsigned char *decryptedData = (unsigned char *)RL_CALLOC(chunk->packedSize, 1);
            
            // WARNING: Implementation dependant! 
            // rrespacker tool appends (salt + nonce + MAC) to encrypted data for convenience,
            // Actually, chunk->packedSize considers those additional elements

            // Required variables for key stretching
            uint8_t key[32] = { 0 };                    // Encryption key
            const uint32_t blocks = 16384;              // Key stretching blocks: 16 megabytes
            const uint32_t iterations = 3;              // Key stretching iterations: 3 iterations
            void *workArea = RL_MALLOC(blocks*1024);    // Key stretching work area
            uint8_t salt[16] = { 0 };                   // Key stretching salt
            
            // Retrieve salt from chunk packed data
            // salt is stored at the end of packed data, before nonce and MAC: salt[16] + nonce[24] + MAC[16]
            memcpy(salt, ((unsigned char *)chunk->data) + (chunk->packedSize - 16 - 24 - 16), 16);

            // Encryption key, generated from user password, using Argon2i algorythm for key stretching (256 bit)
            crypto_argon2i(key, 32, workArea, blocks, iterations, (uint8_t *)rresGetCipherPassword(), 16, salt, 16);

            // Wipe key generation secrets, they are no longer needed
            crypto_wipe(salt, 16);
            RL_FREE(workArea);
            
            // Required variables for decryption and message authentication
            uint8_t nonce[24] = { 0 };                  // nonce used on encryption, unique to processed file
            uint8_t mac[16] = { 0 };                    // Message Authentication Code generated on ecryption
            
            // Retrieve nonce and MAC from chunk packed data
            // nonce and MAC are stored at the end of packed data, after salt: salt[16] + nonce[24] + MAC[16]
            memcpy(nonce, ((unsigned char *)chunk->data) + (chunk->packedSize - 16 - 24), 24);
            memcpy(mac, ((unsigned char *)chunk->data) + (chunk->packedSize - 16), 16);

            // Message decryption requires key, nonce and MAC
            int decryptResult = crypto_unlock(decryptedData, key, nonce, mac, chunk->data, (chunk->packedSize - 16 - 24 - 16));

            // Wipe secrets if they are no longer needed
            crypto_wipe(nonce, 24);
            crypto_wipe(key, 32);

            if (decryptResult == 0)    // Decrypted successfully!
            {
                RRES_FREE(chunk->data);
                chunk->data = decryptedData;
                chunk->packedSize -= (16 + 24 + 16);    // We remove additional data size from packed size
            }
            else if (decryptResult == -1) result = 2;   // Wrong password or message corrupted
#else
            result = 1;     // Decryption algorithm not supported
#endif
        }
        else result = 1;    // Decryption algorithm not supported
    }

    // STEP 2: If decryption was successful, try to decompress data
    if (result == 0) && (chunk->compType != RRES_COMP_NONE)
    {
        // Decompress data
        if (chunk->compType == RRES_COMP_DEFLATE)
        {
            int uncompDataSize = 0;
            unsigned char *uncompData = (unsigned char *)RL_MALLOC(chunk->baseSize);
            uncompData = DecompressData(chunk->data, chunk->packedSize, &uncompDataSize);
            
            if (uncompDataSize > 0)     // Decompression successful
            {
                RRES_FREE(chunk->data);
                chunk->data = uncompData;
                chunk->packedSize = uncompDataSize;
            }
            else result = 4;            // Decompression process failed

            // Security check, uncompDataSize must match the provided chunk->baseSize
            if (uncompDataSize != chunk->baseSize) RRES_LOG("WARNING: Decompressed data could be corrupted, unexpected size\n");

            RL_FREE(compData);
        }
        else if (chunk->compType == RRES_COMP_LZ4)
        {
#if defined(RRES_SUPPORT_COMPRESSION_LZ4)
            int uncompDataSize = 0;
            unsigned char *uncompData = (unsigned char *)RL_MALLOC(chunk->baseSize);
            uncompDataSize = LZ4_decompress_safe(chunk->data, uncompData, chunk->packedSize, chunk->baseSize);

            if (uncompDataSize > 0)     // Decompression successful
            {
                RRES_FREE(chunk->data);
                chunk->data = uncompData;
                chunk->packedSize = uncompDataSize;
            }
            else result = 4;            // Decompression process failed
            
            // WARNING: Decompression could be successful but not the original message size returned
            if (uncompDataSize != chunk->baseSize) RRES_LOG("WARNING: Decompressed data could be corrupted, unexpected size\n");
#else
            result = 3;     // Compression algorithm not supported
#endif
        }
        else if (chunk->compType == RRES_COMP_QOI)
        {
            // TODO.
        }
        else result = 3;    // Compression algorithm not supported
    }
    
    // Show some log info about the decompression/decryption process
    if ((chunk->cipherType != RRES_CIPHER_NONE) || (chunk->compType != RRES_COMP_NONE))
    {
        switch (result)
        {
            case 0: RRES_LOG("INFO: %s: Chunk data decompressed/decrypted successfully\n", GetFourCCFromType(chunk.type)); break;
            case 1: RRES_LOG("WARNING: %s: Chunk data encryption algorithm not supported\n", GetFourCCFromType(chunk.type)); break;
            case 2: RRES_LOG("WARNING: %s: Chunk data decryption failed, wrong password provided\n", GetFourCCFromType(chunk.type)); break;
            case 3: RRES_LOG("WARNING: %s: Chunk data compression algorithm not supported\n", GetFourCCFromType(chunk.type)); break;
            case 4: RRES_LOG("WARNING: %s: Chunk data decompression failed\n", GetFourCCFromType(chunk.type)); break;
            default: break;
        }
    }
    else RRES_LOG("INFO: %s: Chunk does not require data decompression/decryption\n", GetFourCCFromType(chunk.type));

    return result;
}

// Return FourCC from resource type, useful for log info
static const char *GetFourCCFromType(unsigned int type)
{
    switch (type)
    {
        case RRES_DATA_NULL: return "NULL";         // Reserved for empty chunks, no props/data
        case RRES_DATA_RAW: return "RAWD";          // Raw file data, input file is not processed, just packed as is
        case RRES_DATA_TEXT: return "TEXT";         // Text file data, byte data extracted from text file
        case RRES_DATA_IMAGE: return "IMGE";        // Image file data, pixel data extracted from image file
        case RRES_DATA_WAVE: return "WAVE";         // Audio file data, samples data extracted from audio file
        case RRES_DATA_VERTEX: return "VRTX";       // Vertex file data, extracted from a mesh file
        case RRES_DATA_GLYPH_INFO: return "FNTG";   // Font glyphs info, generated from an input font file
        case RRES_DATA_LINK: return "LINK";         // External linked file, filepath as provided on file input
        case RRES_DATA_DIRECTORY: return "CDIR";    // Central directory for input files relation to resource chunks
        default: break;
    }
}

#endif // RRES_RAYLIB_IMPLEMENTATION
